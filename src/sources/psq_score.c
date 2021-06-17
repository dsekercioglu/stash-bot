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

#include "imath.h"
#include "psq_score.h"

scorepair_t PsqScore[PIECE_NB][SQUARE_NB];
const score_t PieceScores[PHASE_NB][PIECE_NB] = {
    {
        0, PAWN_MG_SCORE, KNIGHT_MG_SCORE, BISHOP_MG_SCORE, ROOK_MG_SCORE, QUEEN_MG_SCORE, 0, 0,
        0, PAWN_MG_SCORE, KNIGHT_MG_SCORE, BISHOP_MG_SCORE, ROOK_MG_SCORE, QUEEN_MG_SCORE, 0, 0
    },
    {
        0, PAWN_EG_SCORE, KNIGHT_EG_SCORE, BISHOP_EG_SCORE, ROOK_EG_SCORE, QUEEN_EG_SCORE, 0, 0,
        0, PAWN_EG_SCORE, KNIGHT_EG_SCORE, BISHOP_EG_SCORE, ROOK_EG_SCORE, QUEEN_EG_SCORE, 0, 0
    }
};

#define S SPAIR

const scorepair_t PawnBonus[RANK_NB][FILE_NB] = {
    { },
    { S(-10, 22), S(-14, 10), S( -5, 15), S( -6, 15), S( -1, 34), S( 37, 18), S( 37, -0), S(  6,-17) },
    { S( -7, 13), S(-19, 15), S( -7, 14), S( -6,  1), S(  6, 19), S( 11, 12), S( 15, -7), S( 11,-14) },
    { S(-16, 16), S( -7, 15), S( -3, -3), S(  2,-12), S(  8,-13), S( 11,  6), S( 23, -5), S(  9,-14) },
    { S(-10, 44), S( -3, 33), S(  3, 16), S( 19,-18), S( 37,-18), S( 67,-22), S( 28,  0), S( 28, -2) },
    { S( -4, 88), S( 23, 65), S( 36, 37), S( 39, -3), S( 41,-22), S(122,  3), S( 48, 29), S( 47, 30) },
    { S( 62, 56), S( 92, 38), S( 68, -5), S(133,-55), S(132,-63), S( 26,-70), S(-79,-19), S(-26, 31) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -55, -38), S( -14, -21), S( -16, -24), S(  -6,  -6) },
        { S(  -1, -14), S( -24,  -1), S( -16,   6), S(  -9,   8) },
        { S( -20, -11), S(  -8,   9), S( -11,  25), S(   5,  35) },
        { S(   2,  27), S(  27,  34), S(   4,  51), S(   4,  58) },
        { S(  33,  39), S(   9,  43), S(  17,  55), S(  15,  69) },
        { S(  21,  23), S(  10,  48), S(  15,  67), S(  11,  54) },
        { S(   2,   6), S(  -3,  26), S(  30,  21), S(  57,  43) },
        { S(-135,  32), S( -55,  81), S( -68,  66), S(  51,  59) }
    },

    // Bishop
    {
        { S(  20,  -8), S(  23,  22), S( -12,  15), S( -20,  30) },
        { S( -12,   8), S(   1,   8), S(   1,  16), S( -11,  16) },
        { S(  -0,  17), S(  -8,  41), S(  -1,  28), S(  -1,  39) },
        { S(   5,  19), S(  -0,  19), S(   1,  45), S(  13,  49) },
        { S( -10,  32), S(   6,  49), S(  23,  30), S(  23,  49) },
        { S(  17,  26), S(  19,  53), S(  27,  55), S(  21,  46) },
        { S( -23,  46), S( -54,  61), S(  -2,  51), S( -22,  53) },
        { S( -32,  60), S( -49,  64), S(-127,  71), S( -60,  74) }
    },

    // Rook
    {
        { S( -36,   6), S( -22,  10), S( -23,   9), S( -18,  12) },
        { S( -71,  26), S( -23,  -2), S( -24,   9), S( -28,   6) },
        { S( -43,  13), S( -26,  19), S( -38,  29), S( -36,  23) },
        { S( -42,  38), S( -21,  49), S( -26,  47), S( -28,  41) },
        { S(  -8,  53), S(   9,  61), S(  13,  56), S(  18,  50) },
        { S( -23,  73), S(  21,  66), S(  33,  63), S(  36,  55) },
        { S(   2,  84), S( -10,  92), S(  12,  76), S(  20,  83) },
        { S(  12,  79), S(  27,  87), S( -13,  94), S(  20,  73) }
    },

    // Queen
    {
        { S(  23, -53), S(  20, -58), S(   8, -45), S(  16, -41) },
        { S(  22, -29), S(  19, -47), S(  19, -38), S(  18, -34) },
        { S(  35, -23), S(  22,  -9), S(   7,  36), S(   3,  20) },
        { S(  14,  14), S(  25,  38), S(   1,  55), S(  -4,  63) },
        { S(  46,  11), S(  10,  88), S(  -8, 111), S(  -2,  95) },
        { S(  29,  38), S(  21,  71), S(  10, 112), S(   8, 113) },
        { S(  18,  49), S( -26, 102), S(  -4, 116), S( -19, 144) },
        { S(  -8, 100), S(  23,  73), S( -32, 115), S( -20, 131) }
    },

    // King
    {
        { S( 253, -27), S( 257,  40), S( 225,  49), S( 161,  65) },
        { S( 247,  56), S( 245,  84), S( 197, 111), S( 189, 119) },
        { S( 206,  85), S( 233, 109), S( 200, 135), S( 232, 144) },
        { S( 175, 122), S( 262, 137), S( 226, 164), S( 247, 162) },
        { S( 198, 149), S( 266, 182), S( 325, 187), S( 296, 199) },
        { S( 315, 147), S( 353, 179), S( 329, 210), S( 334, 203) },
        { S( 287, 126), S( 262, 226), S( 284, 204), S( 308, 184) },
        { S( 351,-105), S( 381,  78), S( 295,  97), S( 274, 140) }
    }
};

#undef S

void psq_score_init(void)
{
    for (piece_t piece = WHITE_PAWN; piece <= WHITE_KING; ++piece)
    {
        scorepair_t pieceValue = create_scorepair(PieceScores[MIDGAME][piece], PieceScores[ENDGAME][piece]);

        for (square_t square = SQ_A1; square <= SQ_H8; ++square)
        {
            scorepair_t psqEntry;

            if (piece == WHITE_PAWN)
                psqEntry = pieceValue + PawnBonus[sq_rank(square)][sq_file(square)];
            else
            {
                file_t queensideFile = min(sq_file(square), sq_file(square) ^ 7);

                psqEntry = pieceValue + PieceBonus[piece][sq_rank(square)][queensideFile];
            }

            PsqScore[piece][square] = psqEntry;
            PsqScore[opposite_piece(piece)][opposite_sq(square)] = -psqEntry;
        }
    }
}
