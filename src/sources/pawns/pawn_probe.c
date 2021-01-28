/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2021 Morgan Houppin
**
**    Stash is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    Stash is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "lazy_smp.h"
#include "pawns.h"

enum
{
    BackwardPenalty = SPAIR(-7, -8),
    StragglerPenalty = SPAIR(-18, -14),
    DoubledPenalty = SPAIR(-21, -35),
    IsolatedPenalty = SPAIR(-12, -18),
};

const scorepair_t   PassedBonus[RANK_NB] = {
    0,
    SPAIR(-15, 18),
    SPAIR(-21, 21),
    SPAIR(-11, 51),
    SPAIR(28, 71),
    SPAIR(44, 136),
    SPAIR(112, 224),
    0
};

scorepair_t evaluate_passed(pawn_entry_t *entry, color_t us, bitboard_t our_pawns,
            bitboard_t their_pawns)
{
    scorepair_t     ret = 0;

    while (our_pawns)
    {
        square_t    sq = bb_pop_first_sq(&our_pawns);

        // Save the pawn attack span to the entry
        entry->attack_span[us] |= pawn_attack_span_bb(us, sq);

        if ((passed_pawn_span_bb(us, sq) & their_pawns) == 0)
            ret += PassedBonus[relative_sq_rank(sq, us)];
    }

    return (ret);
}

scorepair_t evaluate_backward(const pawn_entry_t *entry, color_t us, bitboard_t our_pawns,
            bitboard_t their_pawns)
{
    color_t     them = not_color(us);
    bitboard_t  stops = relative_shift_up(our_pawns, us);
    bitboard_t  their_attacks = pawns_attacks_bb(their_pawns, them);
    bitboard_t  backward = relative_shift_down(stops & their_attacks & ~entry->attack_span[us], us);
    scorepair_t ret = 0;

    if (!backward)
        return (ret);

    ret += BackwardPenalty * popcount(backward);

    backward &= (us == WHITE) ? (RANK_2_BITS | RANK_3_BITS) : (RANK_6_BITS | RANK_7_BITS);

    if (!backward)
        return (ret);

    bitboard_t  their_files = 0;

    while (their_pawns)
        their_files |= forward_file_bb(them, bb_pop_first_sq(&their_pawns));

    backward &= ~their_files;

    if (!backward)
        return (ret);

    ret += StragglerPenalty * popcount(backward);

    return (ret);
}

scorepair_t evaluate_doubled_isolated(bitboard_t us)
{
    scorepair_t ret = 0;

    for (square_t s = SQ_A2; s <= SQ_H2; ++s)
    {
        bitboard_t  b = us & sq_file_bb(s);

        if (b)
        {
            if (more_than_one(b))
                ret += DoubledPenalty;
            if (!(adjacent_files_bb(s) & us))
                ret += IsolatedPenalty;
        }
    }

    return (ret);
}

pawn_entry_t    *pawn_probe(const board_t *board)
{
    pawn_entry_t   *entry =
        get_worker(board)->pawn_table + (board->stack->pawn_key % PawnTableSize);

    if (entry->key == board->stack->pawn_key)
        return (entry);

    entry->key = board->stack->pawn_key;
    entry->value = 0;
    entry->attack_span[WHITE] = entry->attack_span[BLACK] = 0;

    const bitboard_t    wpawns = piece_bb(board, WHITE, PAWN);
    const bitboard_t    bpawns = piece_bb(board, BLACK, PAWN);

    entry->value += evaluate_passed(entry, WHITE, wpawns, bpawns);
    entry->value -= evaluate_passed(entry, BLACK, bpawns, wpawns);
    entry->value += evaluate_backward(entry, WHITE, wpawns, bpawns);
    entry->value -= evaluate_backward(entry, BLACK, bpawns, wpawns);
    entry->value += evaluate_doubled_isolated(wpawns);
    entry->value -= evaluate_doubled_isolated(bpawns);

    return (entry);
}