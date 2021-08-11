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
    { S( -5, 26), S( -5, 20), S(-15, 14), S( -3, -6), S( -2, 12), S( 32, 20), S( 40,  3), S( 14,-17) },
    { S( -6, 21), S(-21, 21), S(-10,  9), S( -2,  1), S(  2, 16), S(  1, 18), S( 25, -8), S( 12,-13) },
    { S(-11, 24), S(-12, 17), S(  2,-15), S(  5,-26), S( 11,-13), S( 17, -8), S( 13, -6), S( -1, -4) },
    { S(  0, 48), S(  4, 24), S(  2,  8), S( 22,-36), S( 29,-23), S( 40,-27), S( 28, -3), S( 14, 18) },
    { S( -3, 73), S(  6, 65), S( 15, 25), S( 22,-26), S( 27,-30), S(116,-18), S( 71, 11), S( 33, 27) },
    { S( 84, 38), S(108, 46), S( 77,  8), S( 96,-74), S(128,-82), S( 79,-49), S(-84, 38), S(-69, 70) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -62, -15), S( -11, -17), S(  -4, -21), S( -19,  23) },
        { S( -20,  -7), S( -21,  -1), S( -18,  -4), S(  -5,   7) },
        { S( -21, -26), S( -12,  17), S(  -9,  16), S(   3,  40) },
        { S(   0,  14), S(  22,  40), S(   4,  57), S(   5,  60) },
        { S(  10,  22), S(  10,  35), S(  28,  49), S(  10,  63) },
        { S( -22,  27), S(  22,  34), S(  25,  64), S(  28,  48) },
        { S( -17, -13), S( -31,  35), S(  20,  19), S(  58,  17) },
        { S(-146, -14), S(-102,  37), S(-113,  54), S(  18,  26) }
    },

    // Bishop
    {
        { S(  -0,  12), S(  12,  15), S( -10,  17), S( -23,  22) },
        { S(   2,  -3), S(   8,   2), S(   6,   9), S( -13,  21) },
        { S(  -5,  16), S(   9,  18), S(  -4,  32), S(  -0,  39) },
        { S(  -5,   3), S(  -8,  32), S(  -7,  51), S(  18,  51) },
        { S( -21,  24), S(  -4,  36), S(  19,  34), S(  18,  56) },
        { S(  -1,  25), S(   1,  54), S(  11,  41), S(  21,  26) },
        { S( -27,  45), S( -18,  40), S( -31,  41), S( -16,  35) },
        { S( -69,  47), S( -76,  46), S(-158,  71), S(-102,  59) }
    },

    // Rook
    {
        { S( -30,  -1), S( -28,   6), S( -20,   9), S( -19,  10) },
        { S( -68,  10), S( -45,   7), S( -29,  13), S( -23,   4) },
        { S( -33,   9), S( -35,  24), S( -38,  24), S( -43,  30) },
        { S( -41,  43), S( -25,  52), S( -32,  54), S( -38,  50) },
        { S( -31,  64), S(  -3,  63), S(   9,  52), S(  19,  58) },
        { S(  -9,  65), S(  24,  63), S(  23,  70), S(  48,  44) },
        { S(   2,  76), S(  -6,  79), S(  19,  76), S(  32,  77) },
        { S(   5,  79), S(  17,  65), S(   4,  73), S(  32,  64) }
    },

    // Queen
    {
        { S(  31, -76), S(  25, -75), S(  17, -84), S(  24, -59) },
        { S(  13, -53), S(  13, -46), S(  20, -50), S(  18, -34) },
        { S(  22, -30), S(  12,  -8), S(  12,  22), S(   1,  22) },
        { S(  10,  13), S(   4,  51), S(   4,  49), S(  -5,  86) },
        { S(  23,  29), S(  -5,  99), S(  13,  89), S(  -4, 132) },
        { S(  19,  58), S(  38,  68), S(  -3, 132), S(  -3, 132) },
        { S(   1,  77), S( -38,  97), S(   9, 107), S( -36, 164) },
        { S( -30,  79), S( -15,  94), S( -21, 120), S(  -8, 122) }
    },

    // King
    {
        { S( 244, -16), S( 274,  37), S( 196,  63), S( 168,  64) },
        { S( 257,  45), S( 243,  86), S( 213, 114), S( 180, 129) },
        { S( 187,  75), S( 236,  98), S( 192, 140), S( 223, 151) },
        { S( 174,  95), S( 235, 142), S( 236, 159), S( 198, 180) },
        { S( 212, 122), S( 276, 158), S( 225, 192), S( 194, 193) },
        { S( 267, 147), S( 338, 186), S( 288, 211), S( 244, 183) },
        { S( 251,  83), S( 328, 182), S( 371, 178), S( 358, 158) },
        { S( 314,-170), S( 435,  79), S( 416, 103), S( 353, 137) }
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
