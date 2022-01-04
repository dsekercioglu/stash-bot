/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2022 Morgan Houppin
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
#include <string.h>
#include "endgame.h"
#include "engine.h"
#include "imath.h"
#include "pawns.h"

#ifdef TUNE
evaltrace_t Trace;
#endif

// Special eval terms

const scorepair_t CastlingBonus = SPAIR(95, -71);
const scorepair_t Initiative = SPAIR(15, 15);

// Passed Pawn eval terms

const scorepair_t PP_OurKingProximity[5] = {
    0,
    SPAIR( -6, 37),
    SPAIR(-20, 29),
    SPAIR(-10,  2),
    SPAIR( -5,-13),
};

const scorepair_t PP_TheirKingProximity[5] = {
    0,
    SPAIR(-31,-99),
    SPAIR( 11,-21),
    SPAIR( -2,  4),
    SPAIR(  0, 20),
};

// King Safety eval terms

const scorepair_t KnightWeight    = SPAIR(  44,  74);
const scorepair_t BishopWeight    = SPAIR(  33,-273);
const scorepair_t RookWeight      = SPAIR(  29,-744);
const scorepair_t QueenWeight     = SPAIR(  68, 656);
const scorepair_t AttackWeight    = SPAIR(  19, 202);
const scorepair_t WeakKingZone    = SPAIR(  12,-421);
const scorepair_t BrokenShelter   = SPAIR(   0,   0);
const scorepair_t SafeKnightCheck = SPAIR(  35,-125);
const scorepair_t SafeBishopCheck = SPAIR(  47,   2);
const scorepair_t SafeRookCheck   = SPAIR(  43,  86);
const scorepair_t SafeQueenCheck  = SPAIR(  46, -11);
const scorepair_t SafetyOffset    = SPAIR( -60,-619);

// Knight eval terms

const scorepair_t ClosedPosKnight[5] = {
    SPAIR(0, 0), SPAIR(0, 0), SPAIR(0, 0), SPAIR(0, 0),
    SPAIR(0, 0)
};

const scorepair_t KnightShielded = SPAIR(5, 21);
const scorepair_t KnightOutpost = SPAIR(6, -23);
const scorepair_t KnightCenterOutpost = SPAIR(21, 1);
const scorepair_t KnightSolidOutpost = SPAIR(8, 33);

// Bishop eval terms

const scorepair_t ClosedPosBishop[5] = {
    SPAIR(0, 0), SPAIR(0, 0), SPAIR(0, 0), SPAIR(0, 0),
    SPAIR(0, 0)
};

const scorepair_t BishopPairBonus = SPAIR(25, 98);
const scorepair_t BishopShielded = SPAIR(7, 18);
const scorepair_t BishopBuried = SPAIR(0, 0);

// Rook eval terms

const scorepair_t RookOnSemiOpenFile = SPAIR(14, 32);
const scorepair_t RookOnOpenFile = SPAIR(37, 7);
const scorepair_t RookXrayQueen = SPAIR(10, 13);
const scorepair_t RookTrapped = SPAIR(0, 0);
const scorepair_t RookBuried = SPAIR(0, 0);

// Mobility terms

const scorepair_t MobilityN[9] = {
    SPAIR( -64,  16), SPAIR( -41, -36), SPAIR( -37,  35), SPAIR( -30,  64),
    SPAIR( -25,  73), SPAIR( -19,  89), SPAIR( -16,  91), SPAIR( -11,  87),
    SPAIR(  -8,  70)
};

const scorepair_t MobilityB[14] = {
    SPAIR( -53, -28), SPAIR( -50, -68), SPAIR( -35, -24), SPAIR( -33,   8),
    SPAIR( -24,  31), SPAIR( -18,  46), SPAIR( -13,  56), SPAIR( -13,  60),
    SPAIR( -11,  65), SPAIR(  -8,  63), SPAIR(  -6,  60), SPAIR(  -1,  53),
    SPAIR(   6,  53), SPAIR(  32,  11)
};

const scorepair_t MobilityR[15] = {
    SPAIR( -97,  39), SPAIR( -48,   8), SPAIR( -41,  70), SPAIR( -44,  89),
    SPAIR( -39, 104), SPAIR( -36, 114), SPAIR( -33, 126), SPAIR( -28, 128),
    SPAIR( -23, 135), SPAIR( -17, 137), SPAIR( -12, 140), SPAIR(  -9, 142),
    SPAIR(  -2, 138), SPAIR(   4, 131), SPAIR(  23, 108)
};

const scorepair_t MobilityQ[28] = {
    SPAIR(-100,-189), SPAIR(  16, 111), SPAIR(   1, 185), SPAIR(  -2, 175),
    SPAIR(  -0, 116), SPAIR(  -3, 109), SPAIR(  -1, 130), SPAIR(  -1, 158),
    SPAIR(   2, 173), SPAIR(   5, 187), SPAIR(   9, 196), SPAIR(  12, 199),
    SPAIR(  15, 203), SPAIR(  18, 203), SPAIR(  18, 208), SPAIR(  19, 205),
    SPAIR(  18, 208), SPAIR(  20, 207), SPAIR(  25, 201), SPAIR(  24, 198),
    SPAIR(  26, 189), SPAIR(  28, 187), SPAIR(  21, 195), SPAIR(  22, 179),
    SPAIR(  45, 159), SPAIR(  -3, 178), SPAIR(  16, 183), SPAIR(  48, 159)
};

typedef struct evaluation_s
{
    bitboard_t kingZone[COLOR_NB];
    bitboard_t mobilityZone[COLOR_NB];
    bitboard_t attacked[COLOR_NB];
    bitboard_t attackedTwice[COLOR_NB];
    bitboard_t attackedBy[COLOR_NB][PIECETYPE_NB];
    int safetyAttackers[COLOR_NB];
    int safetyAttacks[COLOR_NB];
    scorepair_t safetyScore[COLOR_NB];
    int tempos[COLOR_NB];
    int positionClosed;
}
evaluation_t;

bool is_kxk_endgame(const board_t *board, color_t us)
{
    // Weak side has pieces or Pawns, this is not a KXK endgame.

    if (more_than_one(color_bb(board, not_color(us))))
        return (false);

    return (board->stack->material[us] >= ROOK_MG_SCORE);
}

score_t eval_kxk(const board_t *board, color_t us)
{
    // Be careful to avoid stalemating the weak King.

    if (board->sideToMove != us && !board->stack->checkers)
    {
        movelist_t list;

        list_all(&list, board);
        if (movelist_size(&list) == 0)
            return (0);
    }

    square_t winningKsq = get_king_square(board, us);
    square_t losingKsq = get_king_square(board, not_color(us));
    score_t score = board->stack->material[us] + popcount(piecetype_bb(board, PAWN)) * PAWN_MG_SCORE;

    // Push the weak King to the corner.

    score += edge_bonus(losingKsq);

    // Give a bonus for close Kings.

    score += close_bonus(winningKsq, losingKsq);

    // Set the score as winning if we have mating material:
    // - a major piece;
    // - a Bishop and a Knight;
    // - two opposite colored Bishops;
    // - three Knights.
    // Note that the KBNK case has already been handled at this point
    // in the eval, so it's not necessary to worry about it.

    bitboard_t knights = piecetype_bb(board, KNIGHT);
    bitboard_t bishops = piecetype_bb(board, BISHOP);

    if (piecetype_bb(board, QUEEN) || piecetype_bb(board, ROOK)
        || (knights && bishops)
        || ((bishops & DARK_SQUARES) && (bishops & ~DARK_SQUARES))
        || (popcount(knights) >= 3))
        score += VICTORY;

    return (board->sideToMove == us ? score : -score);
}

bool ocb_endgame(const board_t *board)
{
    // Check if there is exactly one White Bishop and one Black Bishop.

    bitboard_t wbishop = piece_bb(board, WHITE, BISHOP);

    if (!wbishop || more_than_one(wbishop))
        return (false);

    bitboard_t bbishop = piece_bb(board, BLACK, BISHOP);

    if (!bbishop || more_than_one(bbishop))
        return (false);

    // Then check that the Bishops are on opposite colored squares.

    bitboard_t dsqMask = (wbishop | bbishop) & DARK_SQUARES;

    return (!!dsqMask && !more_than_one(dsqMask));
}

score_t scale_endgame(const board_t *board, score_t eg)
{
    // Only detect scalable endgames from the side with a positive evaluation.
    // This allows us to quickly filter out positions which shouldn't be scaled,
    // even though they have a theoretical scaling factor in our code (like KNvKPPPP).

    color_t strongSide = (eg > 0) ? WHITE : BLACK, weakSide = not_color(strongSide);
    int factor;
    score_t strongMat = board->stack->material[strongSide], weakMat = board->stack->material[weakSide];
    bitboard_t strongPawns = piece_bb(board, strongSide, PAWN), weakPawns = piece_bb(board, weakSide, PAWN);

    // No Pawns and low material difference, the endgame is either drawn
    // or very difficult to win.

    if (!strongPawns && strongMat - weakMat <= BISHOP_MG_SCORE)
        factor = (strongMat <= BISHOP_MG_SCORE) ? 0 : max((int32_t)(strongMat - weakMat) * 8 / BISHOP_MG_SCORE, 0);

    // OCB endgames: scale based on the number of remaining pieces of the strong side.

    else if (ocb_endgame(board))
        factor = 36 + popcount(color_bb(board, strongSide)) * 6;

    // Rook endgames: drawish if the Pawn advantage is small, and all strong side Pawns
    // are on the same side of the board. Don't scale if the defending King is far from
    // his own Pawns.

    else if (strongMat == ROOK_MG_SCORE && weakMat == ROOK_MG_SCORE
        && (popcount(strongPawns) - popcount(weakPawns) < 2)
        && !!(KINGSIDE_BITS & strongPawns) != !!(QUEENSIDE_BITS & strongPawns)
        && (king_moves(get_king_square(board, weakSide)) & weakPawns))
        factor = 64;

    // Other endgames. Decrease the endgame score as the number of pawns of the strong
    // side gets lower.

    else
        factor = min(128, 96 + 8 * popcount(strongPawns));

    // Be careful to cast to 32-bit integer here before multiplying to avoid overflows.

    eg = (score_t)((int32_t)eg * factor / 128);
    TRACE_FACTOR(factor);

    return (eg);
}

void eval_init(const board_t *board, evaluation_t *eval)
{
    memset(eval, 0, sizeof(evaluation_t));

    square_t wksq = get_king_square(board, WHITE);
    square_t bksq = get_king_square(board, BLACK);

    TRACE_ADD(IDX_PSQT + 48 + (KING - KNIGHT) * 32 + to_sq32(wksq), WHITE, 1);
    TRACE_ADD(IDX_PSQT + 48 + (KING - KNIGHT) * 32 + to_sq32(bksq ^ SQ_A8), BLACK, 1);

    // Set the King Attack zone as the 3x4 square surrounding the King
    // (counting an additional rank in front of the King). Init the attack
    // tables at the same time.

    eval->kingZone[WHITE] = king_moves(bksq);
    eval->kingZone[BLACK] = king_moves(wksq);

    eval->attacked[WHITE] = eval->attackedBy[WHITE][KING] = eval->kingZone[BLACK];
    eval->attacked[BLACK] = eval->attackedBy[BLACK][KING] = eval->kingZone[WHITE];

    eval->kingZone[WHITE] |= shift_down(eval->kingZone[WHITE]);
    eval->kingZone[BLACK] |= shift_up(eval->kingZone[BLACK]);

    bitboard_t occupied = occupancy_bb(board);
    bitboard_t wpawns = piece_bb(board, WHITE, PAWN);
    bitboard_t bpawns = piece_bb(board, BLACK, PAWN);
    bitboard_t wattacks = wpawns_attacks_bb(wpawns);
    bitboard_t battacks = bpawns_attacks_bb(bpawns);

    eval->attackedBy[WHITE][PAWN] = wattacks;
    eval->attackedBy[BLACK][PAWN] = battacks;
    eval->attackedTwice[WHITE] |= eval->attacked[WHITE] & wattacks;
    eval->attackedTwice[BLACK] |= eval->attacked[BLACK] & wattacks;
    eval->attacked[WHITE] |= wattacks;
    eval->attacked[BLACK] |= battacks;

    // Exclude opponent Pawns' attacks from the Mobility and King Attack zones.

    eval->mobilityZone[WHITE] = ~battacks;
    eval->mobilityZone[BLACK] = ~wattacks;
    eval->kingZone[WHITE] &= eval->mobilityZone[WHITE];
    eval->kingZone[BLACK] &= eval->mobilityZone[BLACK];

    // Exclude rammed Pawns and our Pawns on rank 2 and 3 from mobility zone.

    eval->mobilityZone[WHITE] &= ~(wpawns & (shift_down(occupied) | RANK_2_BITS | RANK_3_BITS));
    eval->mobilityZone[BLACK] &= ~(bpawns & (shift_up(occupied) | RANK_6_BITS | RANK_7_BITS));

    // Exclude our King from the mobility zone.

    eval->mobilityZone[WHITE] &= ~square_bb(wksq);
    eval->mobilityZone[BLACK] &= ~square_bb(bksq);

    // Add Pawn attacks on opponent's pieces as tempos.

    eval->tempos[WHITE] = popcount(color_bb(board, BLACK) & ~piecetype_bb(board, PAWN) & wattacks);
    eval->tempos[BLACK] = popcount(color_bb(board, WHITE) & ~piecetype_bb(board, PAWN) & battacks);

    // If not in check, add one tempo to the side to move.

    eval->tempos[board->sideToMove] += !board->stack->checkers;

    // Compute position closedness based on the count of pawns who cannot advance
    // (including pawns with their stop squares controlled)
    bitboard_t fixedPawns = wpawns & shift_down(occupied | battacks);
    fixedPawns |= bpawns & shift_up(occupied | wattacks);

    eval->positionClosed = min(4, popcount(fixedPawns) / 2);
}

scorepair_t evaluate_knights(const board_t *board, evaluation_t *eval, const pawn_entry_t *pe, color_t us)
{
    scorepair_t ret = 0;
    bitboard_t bb = piece_bb(board, us, KNIGHT);
    bitboard_t targets = pieces_bb(board, not_color(us), ROOK, QUEEN);
    bitboard_t ourPawns = piece_bb(board, us, PAWN);
    bitboard_t outpost = RANK_4_BITS | RANK_5_BITS | (us == WHITE ? RANK_6_BITS : RANK_3_BITS);

    while (bb)
    {
        square_t sq = bb_pop_first_sq(&bb);
        bitboard_t sqbb = square_bb(sq);
        bitboard_t b = knight_moves(sq);

        TRACE_ADD(IDX_PIECE + KNIGHT - PAWN, us, 1);
        TRACE_ADD(IDX_PSQT + 48 + (KNIGHT - KNIGHT) * 32 + to_sq32(relative_sq(sq, us)), us, 1);

<<<<<<< HEAD
        // If the Knight is pinned, it has no Mobility squares.
=======
        // Give a bonus/penalty based on how closed the position is

        ret += ClosedPosKnight[eval->positionClosed];
        TRACE_ADD(IDX_KNIGHT_CLOSED_POS + eval->positionClosed, us, 1);

        // If the Knight is pinned, it has no Mobility squares
>>>>>>> ee1aaa6... Prepared code for FRC tuning

        if (board->stack->kingBlockers[us] & sqbb)
            b = 0;

        // Update attack tables.

        eval->attackedBy[us][KNIGHT] |= b;
        eval->attackedTwice[us] |= eval->attacked[us] & b;
        eval->attacked[us] |= b;

        // Bonus for Knight mobility

        ret += MobilityN[popcount(b & eval->mobilityZone[us])];
        TRACE_ADD(IDX_MOBILITY_KNIGHT + popcount(b & eval->mobilityZone[us]), us, 1);

        // Bonus for Knight with a Pawn above it

        if (relative_shift_up(sqbb, us) & ourPawns)
        {
            ret += KnightShielded;
            TRACE_ADD(IDX_KNIGHT_SHIELDED, us, 1);
        }

        // Bonus for Knight on Outpost, with higher scores if the Knight is on
        // a center file, on the 6th rank, or supported by a Pawn.

        if (sqbb & outpost & ~pe->attackSpan[not_color(us)])
        {
            ret += KnightOutpost;
            TRACE_ADD(IDX_KNIGHT_OUTPOST, us, 1);

            if (pawn_moves(sq, not_color(us)) & ourPawns)
            {
                ret += KnightSolidOutpost;
                TRACE_ADD(IDX_KNIGHT_SOLID_OUTPOST, us, 1);
            }

            if (sqbb & CENTER_FILES_BITS)
            {
                ret += KnightCenterOutpost;
                TRACE_ADD(IDX_KNIGHT_CENTER_OUTPOST, us, 1);
            }
        }

        // Bonus for a Knight on King Attack zone

        if (b & eval->kingZone[us])
        {
            eval->safetyAttackers[us] += 1;
            eval->safetyAttacks[us] += popcount(b & eval->kingZone[us]);
            eval->safetyScore[us] += KnightWeight;
            TRACE_ADD(IDX_KS_KNIGHT, us, 1);
            TRACE_ADD(IDX_KS_ATTACK, us, popcount(b & eval->kingZone[us]));
        }

        // Tempo bonus for a Knight attacking the opponent's major pieces

        if (b & targets)
            eval->tempos[us] += popcount(b & targets);
    }
    return (ret);
}

scorepair_t evaluate_bishops(const board_t *board, evaluation_t *eval, color_t us)
{
    scorepair_t ret = 0;
    const bitboard_t occupancy = occupancy_bb(board);
    bitboard_t bb = piece_bb(board, us, BISHOP);
    bitboard_t ourPawns = piece_bb(board, us, PAWN);
    bitboard_t targets = pieces_bb(board, not_color(us), ROOK, QUEEN);

    // Bonus for the Bishop pair

    if (more_than_one(bb))
    {
        ret += BishopPairBonus;
        TRACE_ADD(IDX_BISHOP_PAIR, us, 1);
    }

    while (bb)
    {
        square_t sq = bb_pop_first_sq(&bb);
        bitboard_t sqbb = square_bb(sq);
        bitboard_t b = bishop_moves_bb(sq, occupancy);

        TRACE_ADD(IDX_PIECE + BISHOP - PAWN, us, 1);
        TRACE_ADD(IDX_PSQT + 48 + (BISHOP - KNIGHT) * 32 + to_sq32(relative_sq(sq, us)), us, 1);

        // Give a bonus/penalty based on how closed the position is

        ret += ClosedPosBishop[eval->positionClosed];
        TRACE_ADD(IDX_BISHOP_CLOSED_POS + eval->positionClosed, us, 1);

        // Chess960 pattern. If the Bishop is in the corner with a friendly
        // Pawn blocking its diagonal, give a penalty (higher if the Pawn
        // itself is blocked by another piece).
        if (board->chess960)
        {
            square_t relSq = relative_sq(sq, us);

            if (relSq == SQ_A1 && (ourPawns & square_bb(relative_sq(SQ_B2, us))))
            {
                int scale = empty_square(board, relative_sq(SQ_B3, us)) ? 3 : 4;
                ret += BishopBuried * scale;
                TRACE_ADD(IDX_BISHOP_BURIED, us, scale);
            }

            if (relSq == SQ_H1 && (ourPawns & square_bb(relative_sq(SQ_G2, us))))
            {
                int scale = empty_square(board, relative_sq(SQ_G3, us)) ? 3 : 4;
                ret += BishopBuried * scale;
                TRACE_ADD(IDX_BISHOP_BURIED, us, scale);
            }
        }

        // If the Bishop is pinned, reduce its mobility to all the squares
        // between the King and the pinner.

        if (board->stack->kingBlockers[us] & sqbb)
            b &= LineBits[get_king_square(board, us)][sq];

        // Update attack tables.

        eval->attackedBy[us][BISHOP] |= b;
        eval->attackedTwice[us] |= eval->attacked[us] & b;
        eval->attacked[us] |= b;

        // Bonus for Bishop mobility

        ret += MobilityB[popcount(b & eval->mobilityZone[us])];
        TRACE_ADD(IDX_MOBILITY_BISHOP + popcount(b & eval->mobilityZone[us]), us, 1);

		// Bonus for Bishop with a Pawn above it

        if (relative_shift_up(square_bb(sq), us) & ourPawns)
        {
            ret += BishopShielded;
            TRACE_ADD(IDX_BISHOP_SHIELDED, us, 1);
        }

        // Bonus for a Bishop on King Attack zone

        if (b & eval->kingZone[us])
        {
            eval->safetyAttackers[us] += 1;
            eval->safetyAttacks[us] += popcount(b & eval->kingZone[us]);
            eval->safetyScore[us] += BishopWeight;
            TRACE_ADD(IDX_KS_BISHOP, us, 1);
            TRACE_ADD(IDX_KS_ATTACK, us, popcount(b & eval->kingZone[us]));
        }

        // Tempo bonus for a Bishop attacking the opponent's major pieces

        if (b & targets)
            eval->tempos[us] += popcount(b & targets);
    }
    return (ret);
}

scorepair_t evaluate_rooks(const board_t *board, evaluation_t *eval, color_t us)
{
    scorepair_t ret = 0;
    const bitboard_t occupancy = occupancy_bb(board);
    const bitboard_t ourPawns = piece_bb(board, us, PAWN);
    const bitboard_t theirPawns = piece_bb(board, not_color(us), PAWN);
    const bitboard_t theirQueens = piece_bb(board, not_color(us), QUEEN);
    bitboard_t bb = piece_bb(board, us, ROOK);

    while (bb)
    {
        square_t sq = bb_pop_first_sq(&bb);
        bitboard_t sqbb = square_bb(sq);
        bitboard_t rookFile = sq_file_bb(sq);
        bitboard_t b = rook_moves_bb(sq, occupancy);

        TRACE_ADD(IDX_PIECE + ROOK - PAWN, us, 1);
        TRACE_ADD(IDX_PSQT + 48 + (ROOK - KNIGHT) * 32 + to_sq32(relative_sq(sq, us)), us, 1);

        // If the Rook is pinned, reduce its mobility to all the squares
        // between the King and the pinner.

        if (board->stack->kingBlockers[us] & sqbb)
            b &= LineBits[get_king_square(board, us)][sq];

        // Update attack tables.

        eval->attackedBy[us][ROOK] |= b;
        eval->attackedTwice[us] |= eval->attacked[us] & b;
        eval->attacked[us] |= b;

        // Bonus for a Rook on an open (or semi-open) file

        if (!(rookFile & ourPawns))
        {
            ret += (rookFile & theirPawns) ? RookOnSemiOpenFile : RookOnOpenFile;
            TRACE_ADD((rookFile & theirPawns) ? IDX_ROOK_SEMIOPEN : IDX_ROOK_OPEN, us, 1);
        }

        // Bonus for a Rook on the same file as the opponent's Queen(s)

        if (rookFile & theirQueens)
        {
            ret += RookXrayQueen;
            TRACE_ADD(IDX_ROOK_XRAY_QUEEN, us, 1);
        }

        // Bonus for Rook mobility

        int mobility = popcount(b & eval->mobilityZone[us]);

        ret += MobilityR[mobility];
        TRACE_ADD(IDX_MOBILITY_ROOK + popcount(b & eval->mobilityZone[us]), us, 1);

        // Check for trapped Rooks in the opening

        if (mobility <= 3 && relative_sq_rank(sq, us) <= RANK_3)
        {
            file_t kingFile = sq_file(get_king_square(board, us));

            if ((kingFile <= FILE_D) == (sq_file(sq) < kingFile))
            {
                if (has_castling(us, board->stack->castlings))
                {
                    ret += RookTrapped;
                    TRACE_ADD(IDX_ROOK_TRAPPED, us, 1);
                }
                else
                {
                    ret += RookBuried;
                    TRACE_ADD(IDX_ROOK_BURIED, us, 1);
                }
            }
        }

        // Bonus for a Rook on King Attack zone

        if (b & eval->kingZone[us])
        {
            eval->safetyAttackers[us] += 1;
            eval->safetyAttacks[us] += popcount(b & eval->kingZone[us]);
            eval->safetyScore[us] += RookWeight;
            TRACE_ADD(IDX_KS_ROOK, us, 1);
            TRACE_ADD(IDX_KS_ATTACK, us, popcount(b & eval->kingZone[us]));
        }

        // Tempo bonus for a Rook attacking the opponent's Queen(s)

        if (b & theirQueens)
            eval->tempos[us] += popcount(b & theirQueens);
    }
    return (ret);
}

scorepair_t evaluate_queens(const board_t *board, evaluation_t *eval, color_t us)
{
    scorepair_t ret = 0;
    const bitboard_t occupancy = occupancy_bb(board);
    bitboard_t bb = piece_bb(board, us, QUEEN);

    while (bb)
    {
        square_t sq = bb_pop_first_sq(&bb);
        bitboard_t sqbb = square_bb(sq);
        bitboard_t b = bishop_moves_bb(sq, occupancy) | rook_moves_bb(sq, occupancy);

        TRACE_ADD(IDX_PIECE + QUEEN - PAWN, us, 1);
        TRACE_ADD(IDX_PSQT + 48 + (QUEEN - KNIGHT) * 32 + to_sq32(relative_sq(sq, us)), us, 1);

        // If the Queen is pinned, reduce its mobility to all the squares
        // between the King and the pinner.

        if (board->stack->kingBlockers[us] & sqbb)
            b &= LineBits[get_king_square(board, us)][sq];

        // Update attack tables.

        eval->attackedBy[us][QUEEN] |= b;
        eval->attackedTwice[us] |= eval->attacked[us] & b;
        eval->attacked[us] |= b;

        // Bonus for Queen mobility

        ret += MobilityQ[popcount(b & eval->mobilityZone[us])];
        TRACE_ADD(IDX_MOBILITY_QUEEN + popcount(b & eval->mobilityZone[us]), us, 1);

        // Bonus for a Queen on King Attack zone

        if (b & eval->kingZone[us])
        {
            eval->safetyAttackers[us] += 1;
            eval->safetyAttacks[us] += popcount(b & eval->kingZone[us]);
            eval->safetyScore[us] += QueenWeight;
            TRACE_ADD(IDX_KS_QUEEN, us, 1);
            TRACE_ADD(IDX_KS_ATTACK, us, popcount(b & eval->kingZone[us]));
        }
    }
    return (ret);
}

scorepair_t evaluate_passed_pos(const board_t *board, const pawn_entry_t *entry, color_t us)
{
    scorepair_t ret = 0;
    square_t ourKing = get_king_square(board, us);
    square_t theirKing = get_king_square(board, not_color(us));
    bitboard_t bb = entry->passed[us];

    while (bb)
    {
        square_t sq = bb_pop_first_sq(&bb);

        // Give a bonus/penalty based on how close is our King and their King
        // from the Pawn.

        int ourDistance = min(SquareDistance[ourKing][sq], 4);
        int theirDistance = min(SquareDistance[theirKing][sq], 4);

        ret += PP_OurKingProximity[ourDistance];
        ret += PP_TheirKingProximity[theirDistance];

        TRACE_ADD(IDX_PP_OUR_KING_PROX + ourDistance - 1, us, 1);
        TRACE_ADD(IDX_PP_THEIR_KING_PROX + theirDistance - 1, us, 1);
    }

    return (ret);
}

scorepair_t evaluate_safety(const board_t *board, evaluation_t *eval, color_t us)
{
    // Add a bonus if we have 2 pieces (or more) on the King Attack zone, or
    // one piece with a Queen still on the board.

    if (eval->safetyAttackers[us] >= 1 + !piece_bb(board, us, QUEEN))
    {
        color_t them = not_color(us);
        square_t theirKing = get_king_square(board, them);

        // We define weak squares as squares that we attack where the enemy has
        // no defenders, or only the King as a defender.

        bitboard_t weak = eval->attacked[us] & ~eval->attackedTwice[them]
            & (~eval->attacked[them] | eval->attackedBy[them][KING]);

        // We define safe squares as squares that are attacked (or weak and attacked twice),
        // and where we can land on.

        bitboard_t safe = ~color_bb(board, us)
            & (~eval->attacked[them] | (weak & eval->attackedTwice[us]));

        bitboard_t rookCheckSpan = rook_moves(board, theirKing);
        bitboard_t bishopCheckSpan = bishop_moves(board, theirKing);

        bitboard_t knightChecks = safe & eval->attackedBy[us][KNIGHT] & knight_moves(theirKing);
        bitboard_t bishopChecks = safe & eval->attackedBy[us][BISHOP] & bishopCheckSpan;
        bitboard_t rookChecks   = safe & eval->attackedBy[us][ROOK  ] & rookCheckSpan;
        bitboard_t queenChecks  = safe & eval->attackedBy[us][QUEEN ] & (bishopCheckSpan | rookCheckSpan);

        scorepair_t bonus = eval->safetyScore[us] + SafetyOffset;

        bonus += AttackWeight * eval->safetyAttacks[us];
        bonus += WeakKingZone * popcount(weak & eval->kingZone[us]);

        // Check if the Pawn shelter of the King is partially missing.

        for (file_t f = max(FILE_A, sq_file(theirKing) - 1); f <= min(FILE_H, sq_file(theirKing) + 1); ++f)
            if (!(piece_bb(board, them, PAWN) & file_bb(f) & eval->kingZone[us]))
            {
                bonus += BrokenShelter;
                TRACE_ADD(IDX_KS_BRK_SHELTER, us, 1);
            }

        bonus += SafeKnightCheck * popcount(knightChecks);
        bonus += SafeBishopCheck * popcount(bishopChecks);
        bonus += SafeRookCheck   * popcount(rookChecks);
        bonus += SafeQueenCheck  * popcount(queenChecks);

        TRACE_ADD(IDX_KS_OFFSET, us, 1);
        TRACE_ADD(IDX_KS_WEAK_Z, us, popcount(weak & eval->kingZone[us]));
        TRACE_ADD(IDX_KS_CHECK_N, us, popcount(knightChecks));
        TRACE_ADD(IDX_KS_CHECK_B, us, popcount(bishopChecks));
        TRACE_ADD(IDX_KS_CHECK_R, us, popcount(rookChecks));
        TRACE_ADD(IDX_KS_CHECK_Q, us, popcount(queenChecks));
        TRACE_SAFETY(us, bonus);

        score_t mg = midgame_score(bonus), eg = endgame_score(bonus);

        return (create_scorepair(max(mg, 0) * mg / 256, max(eg, 0) / 16));
    }
    return (0);
}

score_t evaluate(const board_t *board)
{
    TRACE_INIT;

    // Do we have a specialized endgame eval for the current configuration ?
    const endgame_entry_t *entry = endgame_probe(board);

    if (entry != NULL)
        return (entry->func(board, entry->winningSide));

    // Is there a KXK situation ? (lone King vs mating material)

    if (is_kxk_endgame(board, WHITE))
        return (eval_kxk(board, WHITE));
    if (is_kxk_endgame(board, BLACK))
        return (eval_kxk(board, BLACK));

    evaluation_t eval;
    scorepair_t tapered = board->psqScorePair;
    pawn_entry_t *pe;
    score_t mg, eg, score;

    // Bonus for having castling rights in the middlegame.
    // (castled King bonus values more than castling right bonus to incite
    // castling in the opening and quick attacks on an uncastled King)

    if (board->stack->castlings & WHITE_CASTLING)
    {
        tapered += CastlingBonus;
        TRACE_ADD(IDX_CASTLING, WHITE, 1);
    }
    if (board->stack->castlings & BLACK_CASTLING)
    {
        tapered -= CastlingBonus;
        TRACE_ADD(IDX_CASTLING, BLACK, 1);
    }

    eval_init(board, &eval);

    // Add the Pawn structure evaluation.

    pe = pawn_probe(board);
    tapered += pe->value;

    // Add the pieces' evaluation.

    tapered += evaluate_knights(board, &eval, pe, WHITE);
    tapered -= evaluate_knights(board, &eval, pe, BLACK);
    tapered += evaluate_bishops(board, &eval, WHITE);
    tapered -= evaluate_bishops(board, &eval, BLACK);
    tapered += evaluate_rooks(board, &eval, WHITE);
    tapered -= evaluate_rooks(board, &eval, BLACK);
    tapered += evaluate_queens(board, &eval, WHITE);
    tapered -= evaluate_queens(board, &eval, BLACK);

    // Add the Passed Pawn evaluation.

    tapered += evaluate_passed_pos(board, pe, WHITE);
    tapered -= evaluate_passed_pos(board, pe, BLACK);

    // Add the King Safety evaluation.

    tapered += evaluate_safety(board, &eval, WHITE);
    tapered -= evaluate_safety(board, &eval, BLACK);

    // Compute Initiative based on how many tempos each side have. The scaling
    // is quadratic so that hanging pieces that can be captured are easily spotted
    // by the eval.

    int tempoValue = (eval.tempos[WHITE] * eval.tempos[WHITE] - eval.tempos[BLACK] * eval.tempos[BLACK]);
    tapered += Initiative * tempoValue;
    TRACE_ADD(IDX_INITIATIVE, WHITE, tempoValue);

    TRACE_EVAL(tapered);

    mg = midgame_score(tapered);

    // Scale endgame score based on remaining material + Pawns.

    eg = scale_endgame(board, endgame_score(tapered));

    // Compute the eval by interpolating between the middlegame and endgame scores.

    {
        int phase = 4 * popcount(piecetype_bb(board, QUEEN))
            + 2 * popcount(piecetype_bb(board, ROOK))
            + popcount(piecetypes_bb(board, KNIGHT, BISHOP));

        if (phase >= 24)
        {
            score = mg;
            TRACE_PHASE(24);
        }
        else
        {
            score = mg * phase / 24;
            score += eg * (24 - phase) / 24;
            TRACE_PHASE(phase);
        }
    }

    // Return the score relative to the side to move.

    return (board->sideToMove == WHITE ? score : -score);
}
