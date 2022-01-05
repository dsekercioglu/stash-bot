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
    { S(-10,  6), S( -0, 13), S( -1, 22), S( -2, 20), S( -7, 19), S(  2, 25), S(  6, 10), S( -6, -3) },
    { S(-14,  3), S(-10,  8), S( -8,  9), S( -2,  3), S(  3,  3), S( -9, 10), S( -7,  7), S(-10, -2) },
    { S( -8,  8), S( -7, 11), S(  4, -8), S( 15,-20), S( 14,-23), S( 10, -8), S( -9,  9), S( -7,  6) },
    { S( 13, 18), S(  9, 15), S( 28, -9), S( 47,-30), S( 46,-29), S( 35,-15), S(  7, 12), S(  8, 16) },
    { S(  8, 59), S( 51, 50), S( 62, 16), S( 65,-16), S( 69,-16), S( 82,  5), S( 57, 40), S( 10, 59) },
    { S( 71,  6), S( 72, 18), S( 86,-11), S(100,-42), S(127,-49), S( 84,-18), S(-38, 55), S(-17, 42) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -53, -53), S( -24, -22), S( -18,  -3), S( -16,  13) },
        { S( -14,   5), S( -28,   7), S( -11,  -3), S(  -1,  18) },
        { S( -21, -17), S(  -7,   4), S(   3,  26), S(   6,  40) },
        { S(   4,  16), S(  18,  31), S(  18,  52), S(  20,  62) },
        { S(  -3,  12), S(  13,  38), S(  29,  53), S(  26,  71) },
        { S( -28,  11), S(  18,  19), S(  21,  61), S(  26,  56) },
        { S( -26, -18), S( -29,   9), S(  31,   5), S(  54,  19) },
        { S(-183, -57), S(-132,  22), S(-128,  47), S( -10,   6) }
    },

    // Bishop
    {
        { S(   9,  -8), S(  -5,  13), S(  -7,  12), S( -13,  15) },
        { S(   8,  -9), S(   7,  -7), S(   0,  14), S(  -6,  25) },
        { S(   3,   8), S(   7,  29), S(   4,  36), S(   6,  39) },
        { S(  -9,  12), S(   1,  27), S(  13,  52), S(  19,  54) },
        { S( -12,  29), S(  14,  48), S(  14,  44), S(  38,  56) },
        { S( -12,  42), S(  15,  54), S(  25,  50), S(  29,  38) },
        { S( -68,  34), S( -42,  52), S( -15,  46), S( -28,  33) },
        { S( -80,  53), S( -69,  42), S(-164,  58), S(-135,  60) }
    },

    // Rook
    {
        { S( -33,  12), S( -18,  18), S( -13,  22), S( -10,  16) },
        { S(-103,  14), S( -29,   6), S( -24,  11), S( -26,  13) },
        { S( -50,  11), S( -21,  26), S( -35,  25), S( -33,  24) },
        { S( -47,  35), S( -27,  46), S( -33,  47), S( -32,  44) },
        { S( -16,  54), S(   6,  47), S(  13,  48), S(  31,  43) },
        { S( -13,  67), S(  45,  40), S(  26,  54), S(  53,  41) },
        { S( -15,  77), S( -12,  81), S(  18,  73), S(  21,  77) },
        { S(  23,  77), S(  17,  80), S(   1,  86), S(   7,  81) }
    },

    // Queen
    {
        { S(  28, -77), S(  35, -66), S(  33, -67), S(  34, -35) },
        { S(  14, -52), S(  32, -54), S(  39, -58), S(  31, -12) },
        { S(  16, -25), S(  24,  -2), S(  18,  29), S(  19,  21) },
        { S(  12,  22), S(  20,  41), S(   4,  63), S(   3,  92) },
        { S(   6,  50), S(  11,  92), S(   1,  87), S(  -4, 114) },
        { S(  -4,  63), S(   7,  61), S(  -6, 106), S(  -4, 113) },
        { S( -16,  54), S( -66, 104), S( -15, 116), S( -54, 154) },
        { S( -19,  65), S( -29,  95), S( -20, 115), S( -20, 124) }
    },

    // King
    {
        { S( 254, -13), S( 266,  60), S( 226,  89), S( 207,  72) },
        { S( 255,  69), S( 248, 102), S( 226, 123), S( 196, 133) },
        { S( 171,  86), S( 243, 108), S( 255, 128), S( 250, 143) },
        { S( 156,  77), S( 273, 117), S( 270, 144), S( 242, 159) },
        { S( 192, 109), S( 288, 146), S( 280, 161), S( 242, 165) },
        { S( 225, 124), S( 333, 165), S( 326, 161), S( 290, 147) },
        { S( 207,  78), S( 267, 174), S( 302, 153), S( 285, 138) },
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
