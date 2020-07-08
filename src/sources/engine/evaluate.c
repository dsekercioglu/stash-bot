/*
**	Stash, a UCI chess playing engine developed from scratch
**	Copyright (C) 2019-2020 Morgan Houppin
**
**	Stash is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	Stash is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include "engine.h"
#include "imath.h"
#include "info.h"
#include "pawns.h"

enum
{
	CastlingBonus = SPAIR(100, 0),
	Initiative = 15,

	MobilityN_Base = SPAIR(-50, -60),
	MobilityN_Quad = SPAIR(75, 80),
	MobilityB_Base = SPAIR(-40, -45),
	MobilityB_Quad = SPAIR(115, 120),
	MobilityR_Base = SPAIR(-45, -60),
	MobilityR_Quad = SPAIR(90, 190),
	MobilityQ_Base = SPAIR(-25, -40),
	MobilityQ_Quad = SPAIR(115, 210),

	PawnWeight = 10,
	MinorWeight = 20,
	RookWeight = 40,
	QueenWeight = 80,
	SafetyScale = 2,
	SafetyRatio = SPAIR(2, 1),

	BishopPairBonus = SPAIR(30, 50),
	KnightPairPenalty = SPAIR(-15, -35),
	RookPairPenalty = SPAIR(-20, -40),
	NonPawnBonus = SPAIR(32, 48)
};

const int	AttackWeights[8] = {
	0, 0, 50, 75, 88, 94, 97, 99
};

scorepair_t	mobility_value(scorepair_t base, scorepair_t delta, int moves, int max)
{
	int		quad = max * max;
	int		diff = max - moves;
	return (base + scorepair_divide(delta * (quad - diff * diff), quad));
}

scorepair_t	evaluate_material(const board_t *board, color_t c)
{
	scorepair_t			ret = 0;
	const bitboard_t	b = board->color_bits[c];

	if (more_than_one(b & board->piecetype_bits[BISHOP]))
		ret += BishopPairBonus;

	if (more_than_one(b & board->piecetype_bits[KNIGHT]))
		ret += KnightPairPenalty;

	if (more_than_one(b & board->piecetype_bits[ROOK]))
		ret += RookPairPenalty;

	ret += NonPawnBonus * popcount(b & ~board->piecetype_bits[PAWN]);

	return (ret);
}

scorepair_t	evaluate_mobility(const board_t *board, color_t c)
{
	scorepair_t		ret = 0;
	int				wattacks = 0;
	int				attackers = 0;
	bitboard_t		king_zone;
	bitboard_t		unsafe;

	const bitboard_t	occupancy = board->piecetype_bits[ALL_PIECES];

	if (c == WHITE)
	{
		king_zone = king_moves(board->piece_list[BLACK_KING][0]);
		king_zone |= shift_down(king_zone);
		unsafe = black_pawn_attacks(board->piecetype_bits[PAWN] & board->color_bits[BLACK]);
	}
	else
	{
		king_zone = king_moves(board->piece_list[WHITE_KING][0]);
		king_zone |= shift_up(king_zone);
		unsafe = white_pawn_attacks(board->piecetype_bits[PAWN] & board->color_bits[WHITE]);
	}
	king_zone &= ~unsafe;
	
	const square_t *list = board->piece_list[create_piece(c, KNIGHT)];

	for (square_t sq = *list; sq != SQ_NONE; sq = *++list)
	{
		bitboard_t	b = knight_moves(sq);

		if (b & king_zone)
		{
			attackers++;
			wattacks += popcount(b & king_zone) * MinorWeight;
		}

		ret += mobility_value(MobilityN_Base, MobilityN_Quad,
			popcount(b & ~unsafe), 8);
	}

	list = board->piece_list[create_piece(c, BISHOP)];
	for (square_t sq = *list; sq != SQ_NONE; sq = *++list)
	{
		bitboard_t	b = bishop_move_bits(sq, occupancy);

		if (b & king_zone)
		{
			attackers++;
			wattacks += popcount(b & king_zone) * MinorWeight;
		}

		ret += mobility_value(MobilityB_Base, MobilityB_Quad,
			popcount(b & ~unsafe), 13);
	}

	list = board->piece_list[create_piece(c, ROOK)];
	for (square_t sq = *list; sq != SQ_NONE; sq = *++list)
	{
		bitboard_t	b = rook_move_bits(sq, occupancy);

		if (b & king_zone)
		{
			attackers++;
			wattacks += popcount(b & king_zone) * RookWeight;
		}

		ret += mobility_value(MobilityR_Base, MobilityR_Quad,
			popcount(b & ~unsafe), 14);
	}

	list = board->piece_list[create_piece(c, QUEEN)];
	for (square_t sq = *list; sq != SQ_NONE; sq = *++list)
	{
		bitboard_t	b = bishop_move_bits(sq, occupancy)
			| rook_move_bits(sq, occupancy);

		if (b & king_zone)
		{
			attackers++;
			wattacks += popcount(b & king_zone) * QueenWeight;
		}

		ret += mobility_value(MobilityQ_Base, MobilityQ_Quad,
			popcount(b & ~unsafe), 27);
	}

	if (attackers >= 8)
		ret += scorepair_divide(SafetyRatio * wattacks, SafetyScale);
	else
		ret += scorepair_divide(SafetyRatio * (wattacks * AttackWeights[attackers] / 100), SafetyScale);

	return (ret);
}

score_t		evaluate(const board_t *board)
{
	scorepair_t		eval = board->psq_scorepair;

	if (board->stack->castlings & WHITE_CASTLING)
		eval += CastlingBonus;
	if (board->stack->castlings & BLACK_CASTLING)
		eval -= CastlingBonus;

	eval += evaluate_pawns(board);
	eval += evaluate_material(board, WHITE);
	eval -= evaluate_material(board, BLACK);
	eval += evaluate_mobility(board, WHITE);
	eval -= evaluate_mobility(board, BLACK);

	score_t		mg = midgame_score(eval);
	score_t		eg = endgame_score(eval);
	int			piece_count = popcount(board->piecetype_bits[ALL_PIECES]);
	score_t		score;

	if (piece_count <= 7)
	{
		// Insufficient material check.

		int		pieces = popcount(board->color_bits[WHITE]);

		if (eg > 0)
		{
			if (pieces == 1)
				return (0);
			else if (pieces == 2 && board_colored_pieces(board, WHITE, KNIGHT, BISHOP))
				return (0);
		}

		pieces = piece_count - pieces;

		if (eg < 0)
		{
			if (pieces == 1)
				return (0);
			else if (pieces == 2 && board_colored_pieces(board, BLACK, KNIGHT, BISHOP))
				return (0);
		}
	}


	if (piece_count <= 16)
		score = eg;
	else
		score = (eg * (32 - piece_count) + mg * (piece_count - 16)) / 16;

	return (Initiative + (board->side_to_move == WHITE ? score : -score));
}
