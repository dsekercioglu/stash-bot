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
    { S(-11,  9), S( -5, 10), S(-24, 20), S( -0, 12), S( -6, 12), S( 17, 17), S( 11, 20), S(-10, -5) },
    { S( -0, -2), S(-18, 13), S( -8, 10), S( -6, -0), S(  4, 10), S(-11, 18), S(  4,  1), S( -5,  2) },
    { S( -1, 21), S( -6,  8), S(  1,-13), S(  9,-22), S(  8,-16), S( 12, -3), S( -2,  3), S( -0,  0) },
    { S(  9, 27), S(  8, 11), S( 15, -8), S( 36,-30), S( 42,-28), S( 44,-14), S( 13,  9), S(  9, 20) },
    { S( 14, 63), S( 46, 46), S( 60, 15), S( 64,-20), S( 65,-19), S( 89, 10), S( 60, 40), S( 10, 56) },
    { S( 72,  9), S( 71, 18), S( 87, -9), S(101,-41), S(126,-50), S( 84,-18), S(-38, 55), S(-18, 42) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -53, -53), S( -23, -22), S( -21,  -4), S( -16,  12) },
        { S( -20,   5), S( -23,  10), S(  -4,  -5), S(  -1,  14) },
        { S( -21, -17), S(  -0,   8), S(   1,  16), S(  16,  44) },
        { S(  -3,  14), S(  22,  33), S(  14,  50), S(  16,  61) },
        { S(  -3,  15), S(  10,  38), S(  26,  55), S(  19,  68) },
        { S( -28,  10), S(  19,  19), S(  23,  62), S(  30,  59) },
        { S( -24, -16), S( -30,  10), S(  32,   7), S(  56,  22) },
        { S(-183, -57), S(-132,  22), S(-127,  48), S( -10,   7) }
    },

    // Bishop
    {
        { S(  11,  -8), S(  -8,  12), S( -14,  11), S( -15,  14) },
        { S(  13,  -8), S(  14,  -4), S(   4,  11), S(  -4,  27) },
        { S(   3,  10), S(   9,  26), S(   4,  32), S(  -0,  43) },
        { S(  -5,  11), S(   8,  29), S(   1,  51), S(  18,  56) },
        { S(  -9,  30), S(   3,  45), S(  15,  42), S(  34,  58) },
        { S(  -4,  42), S(  17,  56), S(  23,  48), S(  29,  37) },
        { S( -66,  34), S( -43,  50), S( -16,  46), S( -28,  35) },
        { S( -80,  53), S( -69,  43), S(-164,  58), S(-135,  61) }
    },

    // Rook
    {
        { S( -30,   7), S( -27,  16), S( -20,  20), S( -13,   9) },
        { S( -93,  16), S( -30,   7), S( -25,   8), S( -25,  11) },
        { S( -51,  13), S( -24,  23), S( -39,  24), S( -35,  21) },
        { S( -49,  39), S( -29,  45), S( -31,  50), S( -28,  46) },
        { S( -14,  58), S(   5,  53), S(  14,  51), S(  28,  42) },
        { S( -13,  67), S(  43,  42), S(  27,  59), S(  56,  43) },
        { S(  -9,  80), S( -11,  82), S(  22,  76), S(  21,  77) },
        { S(  23,  76), S(  16,  79), S(   1,  84), S(   6,  77) }
    },

    // Queen
    {
        { S(  32, -76), S(  29, -66), S(  30, -69), S(  31, -39) },
        { S(  19, -51), S(  29, -53), S(  31, -57), S(  27, -17) },
        { S(  18, -25), S(  21,  -4), S(  16,  27), S(   7,  18) },
        { S(  17,  23), S(  19,  41), S(   1,  63), S(   2,  94) },
        { S(  13,  50), S(  11,  93), S(   4,  89), S(   0, 116) },
        { S(   5,  65), S(  10,  62), S(  -2, 108), S(  -4, 114) },
        { S( -10,  57), S( -63, 106), S( -15, 117), S( -51, 156) },
        { S( -20,  65), S( -29,  95), S( -21, 115), S( -21, 123) }
    },

    // King
    {
        { S( 248, -12), S( 278,  61), S( 216,  87), S( 210,  70) },
        { S( 252,  64), S( 253, 103), S( 227, 121), S( 198, 132) },
        { S( 169,  85), S( 243, 109), S( 253, 127), S( 247, 140) },
        { S( 156,  80), S( 272, 116), S( 269, 142), S( 242, 156) },
        { S( 192, 111), S( 288, 147), S( 279, 161), S( 242, 167) },
        { S( 226, 126), S( 333, 166), S( 325, 162), S( 290, 149) },
        { S( 207,  79), S( 267, 175), S( 303, 154), S( 285, 137) },
        { S( 274,-160), S( 345,  36), S( 315,  62), S( 263,  90) }
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
