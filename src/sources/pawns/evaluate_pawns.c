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

#include "imath.h"
#include "pawns.h"

enum
{
	ZeroEightPawns = SPAIR(-10, -10),
	PawnsInCenter = SPAIR(14, 0),
	PawnsAttackingCenter = SPAIR(9, 8),

	BackwardPenalty = SPAIR(-8, -20),
	StragglerPenalty = SPAIR(-12, -28),
	DoubledPenalty = SPAIR(-8, -7),
	IsolatedPenalty = SPAIR(-22, -20),

	PawnChain = SPAIR(4, 2),
	PawnShield = SPAIR(27, 0),

	MyKingProximity = SPAIR(16, 16),
	OppKingProximity = SPAIR(-10, -10),
};

const scorepair_t	PassedRankBonus[RANK_NB] = {
	0, 0, 0, SPAIR(18, 18), SPAIR(52, 52), SPAIR(108, 108), SPAIR(186, 186), 0
};

const scorepair_t	PassedFileBonus[RANK_NB] = {
	SPAIR(25, 20), SPAIR(11, 15), SPAIR(-14, 5), SPAIR(-14, -7),
	SPAIR(-14, -7), SPAIR(-14, 5), SPAIR(11, 15), SPAIR(25, 20)
};

pawns_cache_t	g_pawns[PawnCacheSize];

bitboard_t	safe_pawn_squares(color_t c, bitboard_t us, bitboard_t them)
{
	bitboard_t	our_double_attacks;
	bitboard_t	our_odd_attacks;
	bitboard_t	their_attacks;
	bitboard_t	their_double_attacks;
	
	if (c == WHITE)
	{
		our_double_attacks = white_pawn_dattacks(us);
		our_odd_attacks = shift_up_left(us) ^ shift_up_right(us);
		their_attacks = black_pawn_attacks(them);
		their_double_attacks = black_pawn_dattacks(them);
	}
	else
	{
		our_double_attacks = black_pawn_dattacks(us);
		our_odd_attacks = shift_down_left(us) ^ shift_down_right(us);
		their_attacks = white_pawn_attacks(them);
		their_double_attacks = white_pawn_dattacks(them);
	}

	return (our_double_attacks | ~their_attacks | (our_odd_attacks & ~their_double_attacks));
}

scorepair_t	evaluate_passers(color_t c, const square_t *ulist, bitboard_t them,
			const board_t *board)
{
	scorepair_t	ret = 0;
	const square_t	my_king = board->piece_list[create_piece(c, KING)][0];
	const square_t	opp_king = board->piece_list[create_piece(them, KING)][0];

	for (square_t sq = *ulist; sq != SQ_NONE; sq = *++ulist)
	{
		if (!(passed_pawn_span(c, sq) & them))
		{
			rank_t	r = relative_square_rank(sq, c);
			ret += PassedFileBonus[file_of_square(sq)] + PassedRankBonus[r];
			ret += MyKingProximity * SquareDistance[sq][my_king];
			ret += OppKingProximity * SquareDistance[sq][opp_king];
		}
	}
	return (ret);
}

scorepair_t	evaluate_chain(bitboard_t us, color_t c)
{
	scorepair_t	ret = 0;
	bitboard_t	b = us;

	while (b)
	{
		square_t	sq = pop_first_square(&b);

		if (PawnMoves[c][sq] & us)
			ret += PawnChain;
	}
	return (ret);
}

scorepair_t	evaluate_backward(color_t c, bitboard_t us, bitboard_t them,
			const square_t *ulist, const square_t *tlist)
{
	bitboard_t	stops = (c == WHITE) ? shift_up(us) : shift_down(us);
	bitboard_t	our_attack_spans = 0;

	for (square_t sq = *ulist; sq != SQ_NONE; sq = *++ulist)
		our_attack_spans |= pawn_attack_span(c, sq);

	bitboard_t	their_attacks = (c == WHITE)
		? black_pawn_attacks(them) : white_pawn_attacks(them);

	bitboard_t	backward = (stops & their_attacks & ~our_attack_spans);
	backward = (c == WHITE) ? shift_down(backward) : shift_up(backward);

	scorepair_t	ret = 0;

	if (!backward)
		return (ret);

	ret += BackwardPenalty * popcount(backward);

	backward &= (c == WHITE) ? (RANK_2_BITS | RANK_3_BITS) : (RANK_6_BITS | RANK_7_BITS);

	if (!backward)
		return (ret);

	bitboard_t	their_files = 0;

	for (square_t sq = *tlist; sq != SQ_NONE; sq = *++tlist)
		their_files |= forward_file_bits(opposite_color(c), sq);

	backward &= ~their_files;

	if (!backward)
		return (ret);

	ret += StragglerPenalty * popcount(backward);

	return (ret);
}

scorepair_t	evaluate_doubled_isolated(bitboard_t us)
{
	scorepair_t	ret = 0;

	for (square_t s = SQ_A2; s <= SQ_H2; ++s)
	{
		bitboard_t	b = us & file_square_bits(s);

		if (b)
		{
			if (more_than_one(b))
				ret += DoubledPenalty;
			if (!(adjacent_files_bits(s) & us))
				ret += IsolatedPenalty;
		}
	}

	return (ret);
}

scorepair_t	evaluate_center(bitboard_t us, color_t c)
{
	scorepair_t	ret = 0;

	int	count = popcount(us);

	if (count == 0 || count == 8)
		ret += ZeroEightPawns;

	ret += PawnsInCenter * popcount(us & CENTER_BITS);
	ret += PawnsAttackingCenter
		* popcount((c == WHITE ? white_pawn_attacks(us) : black_pawn_attacks(us))
		& CENTER_BITS);

	return (ret);
}

scorepair_t	evaluate_shield(bitboard_t us, color_t c, const board_t *board)
{
	bitboard_t	king_zone = board->piecetype_bits[KING] & board->color_bits[c];

	if (c == WHITE)
	{
		king_zone = shift_up(king_zone) | shift_up_left(king_zone)
			| shift_up_right(king_zone);
		king_zone |= shift_up(king_zone);
	}
	else
	{
		king_zone = shift_down(king_zone) | shift_down_left(king_zone)
			| shift_down_right(king_zone);
		king_zone |= shift_down(king_zone);
	}

	return (PawnShield * min(3, popcount(king_zone & us)));
}

scorepair_t	evaluate_pawns(const board_t *board)
{
	score_t	score = 0;

	const bitboard_t	wpawns = board->piecetype_bits[PAWN]
		& board->color_bits[WHITE];
	const bitboard_t	bpawns = board->piecetype_bits[PAWN]
		& board->color_bits[BLACK];
	const square_t		*wlist = board->piece_list[WHITE_PAWN];
	const square_t		*blist = board->piece_list[BLACK_PAWN];

	score += evaluate_backward(WHITE, wpawns, bpawns, wlist, blist);
	score -= evaluate_backward(BLACK, bpawns, wpawns, blist, wlist);
	score += evaluate_passers(WHITE, wlist, bpawns, board);
	score -= evaluate_passers(BLACK, blist, wpawns, board);
	score += evaluate_doubled_isolated(wpawns);
	score -= evaluate_doubled_isolated(bpawns);
	score += evaluate_center(wpawns, WHITE);
	score -= evaluate_center(bpawns, BLACK);
	score += evaluate_chain(wpawns, WHITE);
	score -= evaluate_chain(bpawns, BLACK);
	score += evaluate_shield(wpawns, WHITE, board);
	score -= evaluate_shield(bpawns, BLACK, board);

	return (score);
}
