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

#include <stdlib.h>
#include "engine.h"
#include "imath.h"
#include "info.h"
#include "pawns.h"

enum
{
    // Special eval terms

    CastlingBonus = SPAIR(85, -43),
    Initiative = SPAIR(21, 15),

    // King Safety eval terms

    KnightWeight = SPAIR(22, 6),
    BishopWeight = SPAIR(24, 1),
    RookWeight = SPAIR(47, -3),
    QueenWeight = SPAIR(53, 73),

	// Knight eval terms

    KnightShielded = SPAIR(4, 12),
    KnightOutpost = SPAIR(15, -3),
    KnightCenterOutpost = SPAIR(17, -1),
    KnightSolidOutpost = SPAIR(12, 1),

    // Bishop eval terms

    BishopPairBonus = SPAIR(12, 103),
    BishopShielded = SPAIR(12, 6),

    // Rook eval terms

    RookOnSemiOpenFile = SPAIR(19, 17),
    RookOnOpenFile = SPAIR(38, 16),
    RookXrayQueen = SPAIR(7, 9),

    QueenPhase = 4,
    RookPhase = 2,
    MinorPhase = 1,

    MidgamePhase = 24,
};

const scorepair_t   MobilityN[9] = {
    SPAIR( -79, -68), SPAIR( -43, -71), SPAIR( -27, -16), SPAIR( -27,  26),
    SPAIR(  -4,  31), SPAIR(  -3,  49), SPAIR(   9,  51), SPAIR(  17,  49),
    SPAIR(  24,  32)
};

const scorepair_t   MobilityB[14] = {
    SPAIR( -90, -82), SPAIR( -57,-100), SPAIR( -18, -63), SPAIR( -15, -27),
    SPAIR(  -4,   2), SPAIR(   4,  14), SPAIR(   6,  35), SPAIR(   5,  44),
    SPAIR(   5,  50), SPAIR(   7,  53), SPAIR(   1,  45), SPAIR(  16,  40),
    SPAIR(  50,  35), SPAIR(  47,  26)
};

const scorepair_t   MobilityR[15] = {
    SPAIR( -40, -13), SPAIR( -55, -18), SPAIR( -33,   4), SPAIR( -27,  27),
    SPAIR( -29,  59), SPAIR( -31,  72), SPAIR( -29,  90), SPAIR( -32,  90),
    SPAIR( -23,  92), SPAIR( -16,  98), SPAIR( -20,  99), SPAIR( -14, 107),
    SPAIR(  -9, 105), SPAIR(   1,  95), SPAIR(  57,  62)
};

const scorepair_t   MobilityQ[28] = {
    SPAIR(  -8,-144), SPAIR(  -2,-114), SPAIR(   2, -89), SPAIR(  -7, -67),
    SPAIR( -18, -45), SPAIR(  -9, -14), SPAIR(   6,  32), SPAIR(  13,  76),
    SPAIR(  15, 106), SPAIR(  14, 125), SPAIR(  16, 135), SPAIR(  19, 160),
    SPAIR(  21, 170), SPAIR(  26, 172), SPAIR(  23, 180), SPAIR(  30, 189),
    SPAIR(  28, 191), SPAIR(  25, 199), SPAIR(  30, 203), SPAIR(  19, 199),
    SPAIR(  36, 195), SPAIR(  38, 185), SPAIR(  36, 174), SPAIR(  31, 161),
    SPAIR(  21, 165), SPAIR(  20, 154), SPAIR(  21, 144), SPAIR(  12, 148)
};

const int   AttackRescale[8] = {
    0, 0, 2, 4, 8, 16, 32, 64
};

typedef struct
{
    bitboard_t  king_zone[COLOR_NB];
    bitboard_t  mobility_zone[COLOR_NB];
    int         attackers[COLOR_NB];
    scorepair_t weights[COLOR_NB];
    int         tempos[COLOR_NB];
}
evaluation_t;

bool        is_kxk_endgame(const board_t *board, color_t c)
{
    // Weak side has pieces or pawns, this is not a KXK endgame

    if (more_than_one(color_bb(board, not_color(c))))
        return (false);

    // We have at least a major piece, this is a KXK endgame

    if (piecetypes_bb(board, ROOK, QUEEN))
        return (true);

    bitboard_t  bishop = piecetype_bb(board, BISHOP);

    // We have at least two opposite colored bishops, this is a KXK endgame

    if ((bishop & DARK_SQUARES) && (bishop & ~DARK_SQUARES))
        return (true);

    bitboard_t  knight = piecetype_bb(board, KNIGHT);

    // We have either 3+ knights, or a knight and a bishop, this is a KXK endgame

    if ((knight && bishop) || popcount(knight) >= 3)
        return (true);

    return (false);
}

score_t     eval_kxk(const board_t *board, color_t c)
{
    bitboard_t  pawns = piecetype_bb(board, PAWN);
    score_t     base_score = VICTORY + board->stack->material[c] + popcount(pawns) * PAWN_MG_SCORE;
    square_t    winning_ksq = get_king_square(board, c);
    square_t    losing_ksq = get_king_square(board, not_color(c));

    // Be careful to avoid stalemating the weak king
    if (board->side_to_move != c && !board->stack->checkers)
    {
        movelist_t  list;

        list_all(&list, board);
        if (movelist_size(&list) == 0)
            return (0);
    }

    // KNB endgame, drive the king to the right corner

    if (board->stack->material[c] == KNIGHT_MG_SCORE + BISHOP_MG_SCORE && !pawns)
    {
        square_t    sq = losing_ksq;
        if (piecetype_bb(board, BISHOP) & DARK_SQUARES)
            sq ^= SQ_A8;

        file_t  file = sq_file(sq);
        rank_t  rank = sq_rank(sq);

        base_score += abs(file - rank) * 100;
    }
    else
    {
        file_t  file = sq_file(losing_ksq);
        rank_t  rank = sq_rank(losing_ksq);

        file = min(file, file ^ 7);
        rank = min(rank, rank ^ 7);

        base_score += 720 - (file * file + rank * rank) * 40;
    }

    // Give a bonus for close kings

    base_score += 70 - 10 * SquareDistance[losing_ksq][winning_ksq];

    return (board->side_to_move == c ? base_score : -base_score);
}

void        eval_init(const board_t *board, evaluation_t *eval)
{
    eval->attackers[WHITE] = eval->attackers[BLACK]
        = eval->weights[WHITE] = eval->weights[BLACK] = 0;

    // Set the King Attack zone as the 3x4 square surrounding the king
    // (counting an additional rank in front of the king)

    eval->king_zone[WHITE] = king_moves(get_king_square(board, BLACK));
    eval->king_zone[BLACK] = king_moves(get_king_square(board, WHITE));
    eval->king_zone[WHITE] |= shift_down(eval->king_zone[WHITE]);
    eval->king_zone[BLACK] |= shift_up(eval->king_zone[BLACK]);

    bitboard_t  occupied = occupancy_bb(board);
    bitboard_t  wpawns = piece_bb(board, WHITE, PAWN);
    bitboard_t  bpawns = piece_bb(board, BLACK, PAWN);
    bitboard_t  wattacks = wpawns_attacks_bb(wpawns);
    bitboard_t  battacks = bpawns_attacks_bb(bpawns);

    // Exclude opponent pawns' attacks from the Mobility and King Attack zones

    eval->mobility_zone[WHITE] = ~battacks;
    eval->mobility_zone[BLACK] = ~wattacks;
    eval->king_zone[WHITE] &= eval->mobility_zone[WHITE];
    eval->king_zone[BLACK] &= eval->mobility_zone[BLACK];

    // Exclude rammed pawns and our pawns on rank 2 and 3 from mobility zone

    eval->mobility_zone[WHITE] &= ~(wpawns & (shift_down(occupied) | RANK_2_BITS | RANK_3_BITS));
    eval->mobility_zone[BLACK] &= ~(bpawns & (shift_up(occupied) | RANK_6_BITS | RANK_7_BITS));

    // Add pawn attacks on opponent's pieces as tempos

    eval->tempos[WHITE] = popcount(color_bb(board, BLACK) & ~piecetype_bb(board, PAWN) & wattacks);
    eval->tempos[BLACK] = popcount(color_bb(board, WHITE) & ~piecetype_bb(board, PAWN) & battacks);

    // If not in check, add one tempo to the side to move

    eval->tempos[board->side_to_move] += !board->stack->checkers;
}

scorepair_t evaluate_knights(const board_t *board, evaluation_t *eval, const pawn_entry_t *pe,
            color_t c)
{
    scorepair_t ret = 0;
    bitboard_t  bb = piece_bb(board, c, KNIGHT);
    bitboard_t  targets = pieces_bb(board, not_color(c), ROOK, QUEEN);
    bitboard_t  our_pawns = piece_bb(board, c, PAWN);
    bitboard_t  outpost = RANK_4_BITS | RANK_5_BITS | (c == WHITE ? RANK_6_BITS : RANK_3_BITS);

    while (bb)
    {
        square_t    sq = bb_pop_first_sq(&bb);
        bitboard_t  sqbb = square_bb(sq);
        bitboard_t  b = knight_moves(sq);

        if (board->stack->king_blockers[c] & sqbb)
            b = 0;

        // Bonus for Knight mobility

        ret += MobilityN[popcount(b & eval->mobility_zone[c])];

        // Bonus for Knight with a pawn above it

        if (relative_shift_up(sqbb, c) & our_pawns)
            ret += KnightShielded;

        // Bonus for Knight on Outpost, with higher scores if the Knight is on
        // a center file, on the 6th rank, or supported by a pawn.

        if (sqbb & outpost & ~pe->attack_span[not_color(c)])
        {
            ret += KnightOutpost;

            if (pawn_moves(sq, not_color(c)) & our_pawns)
                ret += KnightSolidOutpost;

            if (sqbb & CENTER_FILES_BITS)
                ret += KnightCenterOutpost;
        }

        // Bonus for a Knight on King Attack zone

        if (b & eval->king_zone[c])
        {
            eval->attackers[c] += 1;
            eval->weights[c] += popcount(b & eval->king_zone[c]) * KnightWeight;
        }

        // Tempo bonus for a Knight attacking the opponent's major pieces

        if (b & targets)
            eval->tempos[c] += popcount(b & targets);
    }
    return (ret);
}

scorepair_t evaluate_bishops(const board_t *board, evaluation_t *eval, color_t c)
{
    scorepair_t         ret = 0;
    const bitboard_t    occupancy = occupancy_bb(board) ^ piece_bb(board, c, QUEEN);
    bitboard_t          bb = piece_bb(board, c, BISHOP);
    bitboard_t          our_pawns = piece_bb(board, c, PAWN);
    bitboard_t          targets = pieces_bb(board, not_color(c), ROOK, QUEEN);

    // Bonus for the Bishop pair

    if (more_than_one(bb))
        ret += BishopPairBonus;

    while (bb)
    {
        square_t    sq = bb_pop_first_sq(&bb);
        bitboard_t  sqbb = square_bb(sq);
        bitboard_t  b = bishop_moves_bb(sq, occupancy);

        if (board->stack->king_blockers[c] & sqbb)
            b &= LineBits[get_king_square(board, c)][sq];

        // Bonus for Bishop mobility

        ret += MobilityB[popcount(b & eval->mobility_zone[c])];

		// Bonus for Bishop with a pawn above it

        if (relative_shift_up(square_bb(sq), c) & our_pawns)
            ret += BishopShielded;

        // Bonus for a Bishop on King Attack zone

        if (b & eval->king_zone[c])
        {
            eval->attackers[c] += 1;
            eval->weights[c] += popcount(b & eval->king_zone[c]) * BishopWeight;
        }

        // Tempo bonus for a Bishop attacking the opponent's major pieces

        if (b & targets)
            eval->tempos[c] += popcount(b & targets);
    }
    return (ret);
}

scorepair_t evaluate_rooks(const board_t *board, evaluation_t *eval, color_t c)
{
    scorepair_t         ret = 0;
    const bitboard_t    occupancy = occupancy_bb(board) ^ pieces_bb(board, c, ROOK, QUEEN);
    const bitboard_t    my_pawns = piece_bb(board, c, PAWN);
    const bitboard_t    their_pawns = piece_bb(board, not_color(c), PAWN);
    const bitboard_t    their_queens = piece_bb(board, not_color(c), QUEEN);
    bitboard_t          bb = piece_bb(board, c, ROOK);

    while (bb)
    {
        square_t    sq = bb_pop_first_sq(&bb);
        bitboard_t  sqbb = square_bb(sq);
        bitboard_t  rook_file = sq_file_bb(sq);
        bitboard_t  b = rook_moves_bb(sq, occupancy);

        if (board->stack->king_blockers[c] & sqbb)
            b &= LineBits[get_king_square(board, c)][sq];

        // Bonus for a Rook on an open (or semi-open) file

        if (!(rook_file & my_pawns))
            ret += (rook_file & their_pawns) ? RookOnSemiOpenFile : RookOnOpenFile;

        // Bonus for a Rook on the same file as the opponent's Queen(s)

        if (rook_file & their_queens)
            ret += RookXrayQueen;

        // Bonus for Rook mobility

        ret += MobilityR[popcount(b & eval->mobility_zone[c])];

        // Bonus for a Rook on King Attack zone

        if (b & eval->king_zone[c])
        {
            eval->attackers[c] += 1;
            eval->weights[c] += popcount(b & eval->king_zone[c]) * RookWeight;
        }

        // Tempo bonus for a Rook attacking the opponent's Queen(s)

        if (b & their_queens)
            eval->tempos[c] += popcount(b & their_queens);
    }
    return (ret);
}

scorepair_t evaluate_queens(const board_t *board, evaluation_t *eval, color_t c)
{
    scorepair_t         ret = 0;
    const bitboard_t    occupancy = occupancy_bb(board);
    bitboard_t          bb = piece_bb(board, c, QUEEN);

    while (bb)
    {
        square_t    sq = bb_pop_first_sq(&bb);
        bitboard_t  sqbb = square_bb(sq);
        bitboard_t  b = bishop_moves_bb(sq, occupancy) | rook_moves_bb(sq, occupancy);

        if (board->stack->king_blockers[c] & sqbb)
            b &= LineBits[get_king_square(board, c)][sq];

        // Bonus for Queen mobility

        ret += MobilityQ[popcount(b & eval->mobility_zone[c])];

        // Bonus for a Queen on King Attack zone

        if (b & eval->king_zone[c])
        {
            eval->attackers[c] += 1;
            eval->weights[c] += popcount(b & eval->king_zone[c]) * QueenWeight;
        }
    }
    return (ret);
}

scorepair_t evaluate_safety(evaluation_t *eval, color_t c)
{
    // Add a bonus if we have 2 pieces (or more) on the King Attack zone

    if (eval->attackers[c] >= 2)
    {
        scorepair_t bonus = eval->weights[c];

        if (eval->attackers[c] < 8)
            bonus -= scorepair_divide(bonus, AttackRescale[eval->attackers[c]]);

        return (bonus);
    }
    return (0);
}

score_t evaluate(const board_t *board)
{
    if (is_kxk_endgame(board, WHITE))
        return (eval_kxk(board, WHITE));
    if (is_kxk_endgame(board, BLACK))
        return (eval_kxk(board, BLACK));

    evaluation_t    eval;
    scorepair_t     tapered = board->psq_scorepair;
    pawn_entry_t    *pe;
    score_t         mg, eg, score;

    // Bonus for having castling rights in the middlegame
    // (castled King bonus values more than castling right bonus to incite
    // castling in the opening and quick attacks on an uncastled King)

    if (board->stack->castlings & WHITE_CASTLING)
        tapered += CastlingBonus;
    if (board->stack->castlings & BLACK_CASTLING)
        tapered -= CastlingBonus;

    eval_init(board, &eval);

    // Add the pawn structure evaluation

    pe = pawn_probe(board);
    tapered += pe->value;

    // Add the pieces' evaluation

    tapered += evaluate_knights(board, &eval, pe, WHITE);
    tapered -= evaluate_knights(board, &eval, pe, BLACK);
    tapered += evaluate_bishops(board, &eval, WHITE);
    tapered -= evaluate_bishops(board, &eval, BLACK);
    tapered += evaluate_rooks(board, &eval, WHITE);
    tapered -= evaluate_rooks(board, &eval, BLACK);
    tapered += evaluate_queens(board, &eval, WHITE);
    tapered -= evaluate_queens(board, &eval, BLACK);

    // Add the King Safety evaluation

    tapered += evaluate_safety(&eval, WHITE);
    tapered -= evaluate_safety(&eval, BLACK);

    // Compute Initiative based on how many tempos each side have. The scaling
    // is quadratic so that hanging pieces that can be captured are easily spotted
    // by the eval

    tapered += Initiative * (eval.tempos[WHITE] * eval.tempos[WHITE] - eval.tempos[BLACK] * eval.tempos[BLACK]);

    mg = midgame_score(tapered);

    // Scale endgame score based on remaining material + pawns
    eg = scale_endgame(board, endgame_score(tapered));

    // Compute the eval by interpolating between the middlegame and endgame scores

    {
        int phase = QueenPhase * popcount(piecetype_bb(board, QUEEN))
            + RookPhase * popcount(piecetype_bb(board, ROOK))
            + MinorPhase * popcount(piecetypes_bb(board, KNIGHT, BISHOP));

        if (phase >= MidgamePhase)
            score = mg;
        else
        {
            score = mg * phase / MidgamePhase;
            score += eg * (MidgamePhase - phase) / MidgamePhase;
        }
    }

    // Return the score relative to the side to move

    return (board->side_to_move == WHITE ? score : -score);
}
