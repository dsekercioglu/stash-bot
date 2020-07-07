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
	SideToMove = SPAIR(24, 27),

	Castling = SPAIR(30, 0),

	OppKingProximity = 16,
	MyKingProximity = -10,

	ConnectedKnights = SPAIR(2, 37),
	KnightProtectedByPawn = SPAIR(7, 16),
	ClosedKnights = 50,

	BishopPair = SPAIR(28, 106),
	BishopProtectedByPawn = SPAIR(6, 12),

	RookOpenFile = SPAIR(60, 2),
	RookSemiOpenFile = SPAIR(27, 24),
	RookXrayQueen = SPAIR(20, 0),
	RookOn7 = SPAIR(0, 11),

	QueenOn7 = SPAIR(0, 35),

	KingPawnlessFlank = SPAIR(17, 95),

	ImbalanceFactor = 180,

	MinorPhase = 1,
	RookPhase = 2,
	QueenPhase = 4,
	TotalPhase = MinorPhase * 8 + RookPhase * 4 + QueenPhase * 2
};

const scorepair_t	MinorAttackBonus[PIECETYPE_NB] = {
	0,
	SPAIR(6, 10), SPAIR(14, 18), SPAIR(14, 18), SPAIR(20, 28), SPAIR(22, 30),
	0, 0
};

const scorepair_t	KnightMobility[9] = {
	SPAIR(-15, -30), SPAIR(-5, -10), SPAIR(-1, -2), SPAIR(2, 4),
	SPAIR(5, 10), SPAIR(7, 14), SPAIR(9, 18), SPAIR(11, 22),
	SPAIR(13, 26)
};

const scorepair_t	BishopMobility[14] = {
	SPAIR(-25, -50), SPAIR(-11, -22), SPAIR(-6, -11), SPAIR(-1, -2),
	SPAIR(3, 6), SPAIR(6, 12), SPAIR(9, 18), SPAIR(12, 24),
	SPAIR(14, 29), SPAIR(17, 34), SPAIR(19, 38), SPAIR(21, 42),
	SPAIR(23, 46), SPAIR(25, 50)
};

const scorepair_t	RookMobility[15] = {
	SPAIR(-10, -50), SPAIR(-4, -22), SPAIR(-2, -11), SPAIR(0, -2),
	SPAIR(2, 6), SPAIR(3, 12), SPAIR(4, 18), SPAIR(5, 24),
	SPAIR(6, 29), SPAIR(8, 34), SPAIR(8, 38), SPAIR(9, 42),
	SPAIR(10, 46), SPAIR(11, 50), SPAIR(12, 54)
};

const scorepair_t	QueenMobility[28] = {
	SPAIR(-10, -50), SPAIR(-6, -30), SPAIR(-5, -22), SPAIR(-4, -16),
	SPAIR(-2, -10), SPAIR(-2, -6), SPAIR(-1, -2), SPAIR(0, 2),
	SPAIR(1, 6), SPAIR(2, 10), SPAIR(2, 13), SPAIR(3, 16),
	SPAIR(3, 19), SPAIR(4, 22), SPAIR(4, 24), SPAIR(5, 27),
	SPAIR(6, 30), SPAIR(6, 32), SPAIR(6, 34), SPAIR(7, 37),
	SPAIR(7, 39), SPAIR(8, 41), SPAIR(8, 43), SPAIR(9, 45),
	SPAIR(9, 47), SPAIR(10, 50), SPAIR(10, 51), SPAIR(10, 53)
};

static const score_t	SafetyTable[9][30] =
{
	{0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
	{0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
	{0,  1,  2,  4,   7,  11,  16,  22,  29,  37,  46,  56,  67,  79,  92, 106, 121, 137, 154, 172, 191, 211, 232, 254, 277, 301, 326, 352, 379, 400},
	{0,  2,  5,  9,  14,  20,  27,  35,  44,  54,  65,  77,  90, 104, 119, 135, 152, 170, 189, 209, 230, 252, 275, 299, 324, 350, 377, 400, 400, 400},
	{0,  4,  8, 13,  19,  26,  34,  43,  53,  64,  76,  89, 103, 118, 134, 151, 169, 188, 208, 229, 251, 274, 298, 323, 349, 376, 400, 400, 400, 400},
	{0,  8, 16, 25,  35,  46,  58,  71,  85, 100, 116, 133, 151, 170, 190, 211, 233, 256, 280, 305, 331, 358, 386, 400, 400, 400, 400, 400, 400, 400},
	{0, 16, 26, 37,  49,  62,  76,  91, 107, 124, 142, 161, 181, 202, 224, 247, 271, 296, 322, 349, 377, 400, 400, 400, 400, 400, 400, 400, 400, 400},
	{0, 32, 44, 57,  71,  86, 102, 119, 137, 156, 176, 197, 219, 242, 266, 291, 317, 344, 372, 400, 400, 400, 400, 400, 400, 400, 400, 400, 400, 400},
	{0, 64, 78, 93, 109, 126, 144, 163, 183, 204, 226, 249, 273, 298, 324, 351, 379, 400, 400, 400, 400, 400, 400, 400, 400, 400, 400, 400, 400, 400}
};

const bitboard_t	Flanking[FILE_NB] = {
	FILE_A_BITS | FILE_B_BITS | FILE_C_BITS,
	FILE_A_BITS | FILE_B_BITS | FILE_C_BITS,
	FILE_A_BITS | FILE_B_BITS | FILE_C_BITS,
	0, 0,
	FILE_F_BITS | FILE_G_BITS | FILE_H_BITS,
	FILE_F_BITS | FILE_G_BITS | FILE_H_BITS,
	FILE_F_BITS | FILE_G_BITS | FILE_H_BITS,
};

INLINED int	eight_distance(int v)
{
	return (v > 4 ? v - 5 : 4 - v);
}

INLINED int	center_distance(square_t square)
{
	file_t		file = file_of_square(square);
	rank_t		rank = rank_of_square(square);

	return (eight_distance(file) + eight_distance(rank));
}

scorepair_t	evaluate_knights(const board_t *board, color_t c)
{
	scorepair_t	ret = 0;
	bitboard_t	knights = board->piecetype_bits[KNIGHT] & board->color_bits[c];
	bitboard_t	pawns = board->piecetype_bits[PAWN] & board->color_bits[c];

	if (more_than_one(knights))
	{
		bitboard_t	knight_moves = 0;
		bitboard_t	b = knights;

		while (b)
			knight_moves |= PseudoMoves[KNIGHT][pop_first_square(&b)];

		if (knight_moves & knights)
			ret += ConnectedKnights;
	}

	const square_t	*list = board->piece_list[create_piece(c, KNIGHT)];
	bitboard_t		pattacks = (c == WHITE) ? white_pawn_attacks(pawns)
		: black_pawn_attacks(pawns);

	for (square_t square = *list; square != SQ_NONE; square = *++list)
		if (pattacks & square_bit(square))
			ret += KnightProtectedByPawn;

	bitboard_t	rammed = shift_up(board->piecetype_bits[PAWN] & board->color_bits[WHITE])
		& board->piecetype_bits[PAWN] & board->color_bits[BLACK];

	int			ram_pairs = popcount(rammed);
	int			non_ram_count = popcount(board->piecetype_bits[PAWN]) - ram_pairs * 2;

	double		closedness = (ram_pairs * 2.0 + non_ram_count / 2.0) - 8.0;

	if (closedness > 0.0)
	{
		score_t		factor = popcount(knights) * closedness / 8.0 * ClosedKnights;

		ret += create_scorepair(factor, factor);
	}

	return (ret);
}

scorepair_t	evaluate_closedness(const board_t *board)
{
	scorepair_t		ret = 0;

	bitboard_t	rammed = shift_up(board->piecetype_bits[PAWN] & board->color_bits[WHITE])
		& board->piecetype_bits[PAWN] & board->color_bits[BLACK];

	int			ram_pairs = popcount(rammed);
	int			non_ram_count = popcount(board->piecetype_bits[PAWN]) - ram_pairs * 2;
	double		closedness = (ram_pairs * 2.0 + non_ram_count / 2.0) - 8.0;

	if (closedness > 0.0)
	{
		score_t		bonus =
			(board->piece_count[WHITE_KNIGHT] - board->piece_count[BLACK_KNIGHT])
			* closedness / 8.0 * ClosedKnights;

		ret += create_scorepair(bonus, bonus);
	}

	return (ret);
}

scorepair_t	evaluate_bishops(const board_t *board, color_t c)
{
	scorepair_t	ret = 0;
	bitboard_t	bishops = board->piecetype_bits[BISHOP] & board->color_bits[c];
	bitboard_t	pawns = board->piecetype_bits[PAWN] & board->color_bits[c];

	if (more_than_one(bishops))
		ret += BishopPair;

	const square_t	*list = board->piece_list[create_piece(c, BISHOP)];
	bitboard_t		pattacks = (c == WHITE) ? white_pawn_attacks(pawns)
		: black_pawn_attacks(pawns);

	for (square_t square = *list; square != SQ_NONE; square = *++list)
		if (pattacks & square_bit(square))
			ret += BishopProtectedByPawn;

	return (ret);
}

scorepair_t	evaluate_rooks_and_queens(const board_t *board, color_t c)
{
	scorepair_t	ret = 0;
	bitboard_t	my_pawns = board->piecetype_bits[PAWN] & board->color_bits[c];
	bitboard_t	their_pawns = board->piecetype_bits[PAWN]
		& board->color_bits[opposite_color(c)];
	bitboard_t	their_queens = board->piecetype_bits[QUEEN]
		& board->color_bits[opposite_color(c)];

	bool	their_king_is_on_rank_7 = rank_of_square(
		board->piece_list[create_piece(opposite_color(c), KING)][0]) == RANK_7;

	const square_t		*list = board->piece_list[create_piece(c, ROOK)];

	for (square_t square = *list; square != SQ_NONE; square = *++list)
	{
		bitboard_t	file_bb = file_square_bits(square);

		if (!(file_bb & my_pawns))
			ret += (file_bb & their_pawns) ? RookSemiOpenFile : RookOpenFile;

		if (file_bb & their_queens)
			ret += RookXrayQueen;

		if (rank_of_square(square) != RANK_7)
			continue ;

		if (!(their_pawns & RANK_7_BITS) && !their_king_is_on_rank_7)
			continue ;

		ret += RookOn7;
	}

	list = board->piece_list[create_piece(c, QUEEN)];

	for (square_t square = *list; square != SQ_NONE; square = *++list)
	{
		if (rank_of_square(square) != RANK_7)
			continue ;

		if (!(their_pawns & RANK_7_BITS) && !their_king_is_on_rank_7)
			continue ;

		ret += QueenOn7;
	}

	return (ret);
}

scorepair_t	evaluate_king_flank(const board_t *board, color_t c)
{
	square_t	my_king = board->piece_list[create_piece(c, KING)][0];
	bitboard_t	flank = Flanking[file_of_square(my_king)];

	if (flank && !(flank & board->piecetype_bits[PAWN] & board->color_bits[c]))
		return (KingPawnlessFlank);
	return (0);
}

typedef struct
{
	int		king_attacks;
	int		mobility;
	int		king_attackers;
	int		unsafe;
	int		center;
	int		home_rows_attacks;
}		mobility_t;

mobility_t	*compute_mobility(const board_t *board, color_t c)
{
	static mobility_t	mob[PIECETYPE_NB];
	const bitboard_t	occupancy = board->piecetype_bits[ALL_PIECES];
	const bitboard_t	my_pieces = board->color_bits[c];
	const bitboard_t	home_rows = (c == WHITE) ? RANK_7_BITS | RANK_8_BITS
		: RANK_1_BITS | RANK_2_BITS;
	const bitboard_t	opp_pawn_attacks = (c == WHITE)
		? black_pawn_attacks(board->piecetype_bits[PAWN] & board->color_bits[BLACK])
		: white_pawn_attacks(board->piecetype_bits[PAWN] & board->color_bits[WHITE]);

	bitboard_t	king_zone = board->piecetype_bits[KING] & ~my_pieces;

	if (c == WHITE)
	{
		king_zone = shift_down(king_zone) | shift_down_left(king_zone)
			| shift_down_right(king_zone);
		king_zone |= shift_down(king_zone);
	}
	else
	{
		king_zone = shift_up(king_zone) | shift_up_left(king_zone)
			| shift_up_right(king_zone);
		king_zone |= shift_up(king_zone);
	}

	{
		bitboard_t	all_knight_moves = 0;

		const square_t	*list = board->piece_list[create_piece(c, KNIGHT)];
		mob[KNIGHT].king_attacks = 0;
		mob[KNIGHT].king_attackers = 0;
		mob[KNIGHT].center = 0;

		for (square_t square = *list; square != SQ_NONE; square = *++list)
		{
			bitboard_t	b = PseudoMoves[KNIGHT][square] & ~my_pieces;

			all_knight_moves |= b;

			if (b & king_zone)
			{
				mob[KNIGHT].king_attacks += popcount(b & king_zone);
				mob[KNIGHT].king_attackers++;
			}
			mob[KNIGHT].center += popcount(b & CENTER_BITS);
		}
		mob[KNIGHT].mobility = popcount(all_knight_moves);
		mob[KNIGHT].unsafe = popcount(all_knight_moves & opp_pawn_attacks);
		mob[KNIGHT].home_rows_attacks = popcount(all_knight_moves & home_rows);
	}

	{
		bitboard_t	all_bishop_moves = 0;

		const square_t	*list = board->piece_list[create_piece(c, BISHOP)];
		mob[BISHOP].king_attacks = 0;
		mob[BISHOP].king_attackers = 0;
		mob[BISHOP].center = 0;

		for (square_t square = *list; square != SQ_NONE; square = *++list)
		{
			bitboard_t	b = bishop_move_bits(square, occupancy) & ~my_pieces;

			all_bishop_moves |= b;

			if (b & king_zone)
			{
				mob[BISHOP].king_attacks += popcount(b & king_zone);
				mob[BISHOP].king_attackers++;
			}
			mob[BISHOP].center += popcount(b & CENTER_BITS);
		}
		mob[BISHOP].mobility = popcount(all_bishop_moves);
		mob[BISHOP].unsafe = popcount(all_bishop_moves & opp_pawn_attacks);
		mob[BISHOP].home_rows_attacks = popcount(all_bishop_moves & home_rows);
	}

	{
		bitboard_t	all_rook_moves = 0;

		const square_t	*list = board->piece_list[create_piece(c, ROOK)];
		mob[ROOK].king_attacks = 0;
		mob[ROOK].king_attackers = 0;
		mob[ROOK].center = 0;

		for (square_t square = *list; square != SQ_NONE; square = *++list)
		{
			bitboard_t	b = rook_move_bits(square, occupancy) & ~my_pieces;

			all_rook_moves |= b;

			if (b & king_zone)
			{
				mob[ROOK].king_attacks += popcount(b & king_zone);
				mob[ROOK].king_attackers++;
			}
			mob[ROOK].center += popcount(b & CENTER_BITS);
		}
		mob[ROOK].mobility = popcount(all_rook_moves);
		mob[ROOK].unsafe = popcount(all_rook_moves & opp_pawn_attacks);
		mob[ROOK].home_rows_attacks = popcount(all_rook_moves & home_rows);
	}

	{
		bitboard_t	all_queen_moves = 0;

		const square_t	*list = board->piece_list[create_piece(c, QUEEN)];
		mob[QUEEN].king_attacks = 0;
		mob[QUEEN].king_attackers = 0;
		mob[QUEEN].center = 0;

		for (square_t square = *list; square != SQ_NONE; square = *++list)
		{
			bitboard_t	b = (rook_move_bits(square, occupancy)
				| bishop_move_bits(square, occupancy)) & ~my_pieces;

			all_queen_moves |= b;

			if (b & king_zone)
			{
				mob[QUEEN].king_attacks += popcount(b & king_zone);
				mob[QUEEN].king_attackers++;
			}
			mob[QUEEN].center += popcount(b & CENTER_BITS);
		}
		mob[QUEEN].mobility = popcount(all_queen_moves);
		mob[QUEEN].unsafe = popcount(all_queen_moves & opp_pawn_attacks);
		mob[QUEEN].home_rows_attacks = popcount(all_queen_moves & home_rows);
	}

	return (mob);
}

INLINED int	mobility_score(mobility_t *mob)
{
	return (mob->mobility - mob->unsafe * 2 + mob->center / 2);
}

scorepair_t	evaluate_mobility(const board_t *board, color_t c)
{
	scorepair_t		ret = 0;

	int		king_attackers = 0;
	int		king_attacks = 0;

	mobility_t	*mob = compute_mobility(board, c);

	ret += KnightMobility[clamp(mobility_score(mob + KNIGHT), 0, 8)];
	ret += create_scorepair(mob[KNIGHT].home_rows_attacks, 0);
	king_attacks += mob[KNIGHT].king_attacks;
	king_attackers += mob[KNIGHT].king_attackers;

	ret += BishopMobility[clamp(mobility_score(mob + BISHOP), 0, 13)];
	ret += create_scorepair(mob[BISHOP].home_rows_attacks, 0);
	king_attacks += mob[BISHOP].king_attacks;
	king_attackers += mob[BISHOP].king_attackers;

	ret += RookMobility[clamp(mobility_score(mob + ROOK), 0, 14)];
	ret += create_scorepair(mob[ROOK].home_rows_attacks, 0);
	king_attacks += mob[ROOK].king_attacks * 2;
	king_attackers += mob[ROOK].king_attackers;

	ret += QueenMobility[clamp(mobility_score(mob + QUEEN), 0, 27)];
	ret += create_scorepair(mob[QUEEN].home_rows_attacks, 0);
	king_attacks += mob[QUEEN].king_attacks * 4;
	king_attackers += mob[QUEEN].king_attackers;

	king_attackers = min(8, king_attackers);
	king_attacks = min(29, king_attacks);

	score_t		attack_bonus = SafetyTable[king_attackers][king_attacks];

	ret += create_scorepair(attack_bonus, attack_bonus);

	return (ret);
}

scorepair_t	evaluate_minor_attacks(const board_t *board, color_t c)
{
	scorepair_t		ret = 0;

	bitboard_t	our_knights = board->color_bits[c] & board->piecetype_bits[KNIGHT];
	bitboard_t	our_bishops = board->color_bits[c] & board->piecetype_bits[BISHOP];
	bitboard_t	occupancy = board->piecetype_bits[ALL_PIECES];
	bitboard_t	their_pawn_attacks = (c == WHITE)
		? black_pawn_attacks(board->color_bits[BLACK] & board->piecetype_bits[PAWN])
		: white_pawn_attacks(board->color_bits[WHITE] & board->piecetype_bits[PAWN]);
	bitboard_t	their_pieces = board->color_bits[opposite_color(c)]
		& ~board->piecetype_bits[PAWN];

	while (their_pieces)
	{
		square_t	square = pop_first_square(&their_pieces);

		if (square_bit(square) & their_pawn_attacks)
			continue ;

		if ((PseudoMoves[KNIGHT][square] & our_knights)
			|| (bishop_move_bits(square, occupancy) & our_bishops))
			ret += MinorAttackBonus[type_of_piece(piece_on(board, square))];
	}

	return (ret);
}

scorepair_t	evaluate_imbalance(const board_t *board)
{
	score_t		wmat = board->piece_count[WHITE_KNIGHT] * KNIGHT_MG_SCORE
		+ board->piece_count[WHITE_BISHOP] * BISHOP_MG_SCORE
		+ board->piece_count[WHITE_ROOK] * ROOK_MG_SCORE
		+ board->piece_count[WHITE_ROOK] * QUEEN_MG_SCORE;

	score_t		bmat = board->piece_count[BLACK_KNIGHT] * KNIGHT_MG_SCORE
		+ board->piece_count[BLACK_BISHOP] * BISHOP_MG_SCORE
		+ board->piece_count[BLACK_ROOK] * ROOK_MG_SCORE
		+ board->piece_count[BLACK_ROOK] * QUEEN_MG_SCORE;

	if (wmat > bmat)
	{
		score_t		imbalance = 180 * (1.0 - ((double)bmat / wmat));
		return (create_scorepair(imbalance, imbalance));
	}
	else if (bmat > wmat)
	{
		score_t		imbalance = 180 * (1.0 - ((double)wmat / bmat));
		return (create_scorepair(-imbalance, -imbalance));
	}
	else
		return (0);
}

score_t		evaluate_endgame(const board_t *board, score_t eg)
{
	int		pieces = popcount(board->piecetype_bits[ALL_PIECES]);

	if (pieces >= 6)
		return (eg);

	const int wp = board->piece_count[create_piece(WHITE, PAWN)];
	const int wn = board->piece_count[create_piece(WHITE, KNIGHT)];
	const int wb = board->piece_count[create_piece(WHITE, BISHOP)];
	const int wr = board->piece_count[create_piece(WHITE, ROOK)];
	const int bp = board->piece_count[create_piece(BLACK, PAWN)];
	const int bn = board->piece_count[create_piece(BLACK, KNIGHT)];
	const int bb = board->piece_count[create_piece(BLACK, BISHOP)];
	const int br = board->piece_count[create_piece(BLACK, ROOK)];

	if (pieces == 5)
	{
		if (wn == 2)
		{
			if (board->piecetype_bits[PAWN] || board->piecetype_bits[QUEEN])
				return (eg);
			if (bn || bb || br)
				return (0);
		}
		if (bn == 2)
		{
			if (board->piecetype_bits[PAWN] || board->piecetype_bits[QUEEN])
				return (eg);
			if (wn || wb || wr)
				return (0);
		}
		if (wr && br)
			if (board->piecetype_bits[KNIGHT] || board->piecetype_bits[BISHOP])
				return (eg / 16);
		if (wn && wb && (bn || bb || br))
			return (0);
		if (bn && bb && (wn || wb || wr))
			return (0);

		if (wb == 2)
		{
			if (bb)
				return (0);
			if (bn)
				return (eg / 2);
		}
		if (bb == 2)
		{
			if (wb)
				return (0);
			if (wn)
				return (eg / 2);
		}
		if (wr && (bn || bb) && bp)
			return (min(eg, 0));
		if (br && (wn || wb) && wp)
			return (max(eg, 0));
	}
	if (pieces == 4)
	{
		if ((wn || wb) && bp)
			return (min(eg, 0));
		if ((bn || bb) && wp)
			return (max(eg, 0));
	}
	if (pieces == 3)
	{
		if (wn || wb || bn || bb)
			return (0);
	}
	return (eg);
}

score_t		evaluate(const board_t *board)
{
	scorepair_t		eval = board->psq_scorepair;

	eval += (board->stack->castlings & WHITE_CASTLING) ? Castling : 0;
	eval -= (board->stack->castlings & BLACK_CASTLING) ? Castling : 0;
	eval += (board->side_to_move == WHITE) ? SideToMove : -SideToMove;

	eval += evaluate_knights(board, WHITE);
	eval -= evaluate_knights(board, BLACK);
	eval += evaluate_closedness(board);
	eval += evaluate_bishops(board, WHITE);
	eval -= evaluate_bishops(board, BLACK);
	eval += evaluate_rooks_and_queens(board, WHITE);
	eval -= evaluate_rooks_and_queens(board, BLACK);
	eval -= evaluate_king_flank(board, WHITE);
	eval += evaluate_king_flank(board, BLACK);
	eval += evaluate_mobility(board, WHITE);
	eval -= evaluate_mobility(board, BLACK);
	eval += evaluate_minor_attacks(board, WHITE);
	eval -= evaluate_minor_attacks(board, BLACK);
	eval += evaluate_imbalance(board);

	score_t		mg = midgame_score(eval);
	score_t		eg = evaluate_endgame(board, endgame_score(eval));

	int		ncount = popcount(board->piecetype_bits[KNIGHT]);
	int		bcount = popcount(board->piecetype_bits[BISHOP]);
	int		rcount = popcount(board->piecetype_bits[ROOK]);
	int		qcount = popcount(board->piecetype_bits[QUEEN]);

	int		phase = TotalPhase
		- ncount * MinorPhase
		- bcount * MinorPhase
		- rcount * RookPhase
		- qcount * QueenPhase;

	phase = (phase * 256 + (TotalPhase / 2)) / TotalPhase;

	score_t score = ((mg * (256 - phase) + eg * phase) / 256);

	return (board->side_to_move == WHITE ? score : -score);
}
