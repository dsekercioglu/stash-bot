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

#include "lazy_smp.h"
#include "pawns.h"

enum
{
	BackwardPenalty = SPAIR(-7, -8),
	StragglerPenalty = SPAIR(-18, -14),
	DoubledPenalty = SPAIR(-21, -35),
	IsolatedPenalty = SPAIR(-12, -18),

//	CandidateBonus = SPAIR(0, 10),
};

const scorepair_t	PassedBonus[RANK_NB] = {
	0,
	SPAIR(-15, 18),
	SPAIR(-21, 21),
	SPAIR(-11, 51),
	SPAIR(28, 71),
	SPAIR(44, 136),
	SPAIR(112, 224),
	0
};

/*
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
*/
scorepair_t	evaluate_passers(color_t c, bitboard_t us, bitboard_t them)
{
	scorepair_t	ret = 0;

	while (us)
	{
		square_t	sq = pop_first_square(&us);
		if (!(passed_pawn_span(c, sq) & them))
			ret += PassedBonus[relative_square_rank(sq, c)];
	}
	return (ret);
}
/*
scorepair_t	evaluate_candidates(color_t c, bitboard_t us, bitboard_t them)
{
	bitboard_t	rank5 = (c == WHITE) ? RANK_5_BITS : RANK_4_BITS;
	bitboard_t	their_attacks = (c == WHITE) ? black_pawn_attacks(them)
		: white_pawn_attacks(them);

	bitboard_t	our_safe_squares = safe_pawn_squares(c, us, them);
	bitboard_t	safe_attacked = our_safe_squares & their_attacks;
	bitboard_t	their_front_span = (c == WHITE) ? (them >> 8) | (them >> 16)
		: (them << 8) | (them << 16);

	bitboard_t	candidates = us & rank5 & ~their_front_span
		& (c == WHITE ? shift_up(safe_attacked) : shift_down(safe_attacked));

	return (candidates ? CandidateBonus * popcount(candidates) : 0);
}
*/
scorepair_t	evaluate_backward(color_t c, bitboard_t us, bitboard_t them)
{
	bitboard_t	stops = (c == WHITE) ? shift_up(us) : shift_down(us);
	bitboard_t	our_attack_spans = 0;

	bitboard_t	bb = us;
	while (bb)
		our_attack_spans |= pawn_attack_span(c, pop_first_square(&bb));

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

	bb = them;
	while (bb)
		their_files |= forward_file_bits(not_color(c), pop_first_square(&bb));

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

scorepair_t	evaluate_pawns(const board_t *board)
{
	pawns_cache_t	*entry =
		&(get_worker(board)->pawns_cache[board->stack->pawn_key
		& (PawnCacheSize - 1)]);

	if (entry->key == board->stack->pawn_key)
		return (entry->value);

	entry->key = board->stack->pawn_key;
	entry->value = 0;

	const bitboard_t	wpawns = board->piecetype_bits[PAWN]
		& board->color_bits[WHITE];
	const bitboard_t	bpawns = board->piecetype_bits[PAWN]
		& board->color_bits[BLACK];

	entry->value += evaluate_backward(WHITE, wpawns, bpawns);
	entry->value -= evaluate_backward(BLACK, bpawns, wpawns);
	entry->value += evaluate_passers(WHITE, wpawns, bpawns);
	entry->value -= evaluate_passers(BLACK, bpawns, wpawns);
//	entry->value += evaluate_candidates(WHITE, wpawns, bpawns);
//	entry->value -= evaluate_candidates(BLACK, bpawns, wpawns);
	entry->value += evaluate_doubled_isolated(wpawns);
	entry->value -= evaluate_doubled_isolated(bpawns);

	return (entry->value);
}
