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
    { S(-11, 24), S( -7, 22), S(-18, 21), S( -1,  0), S(  0, 17), S( 32, 16), S( 39,  0), S( 10,-16) },
    { S(-10, 16), S(-21, 24), S( -6, 14), S( -4,  5), S(  6, 13), S(  4, 16), S( 23,-13), S( 12,-12) },
    { S(-11, 25), S(-11, 17), S(  3, -4), S(  8,-20), S( 13,-13), S( 18, -6), S( 19, -9), S(  5, -6) },
    { S( -4, 44), S( -4, 26), S(  4,  8), S( 23,-21), S( 38,-17), S( 47,-20), S( 27, -2), S( 11, 12) },
    { S(  6, 65), S(  8, 45), S( 18, 22), S( 23,-15), S( 46,-28), S(123, -8), S( 71, 13), S( 35, 30) },
    { S( 89, 23), S( 86, 21), S( 90,  1), S(100,-53), S(126,-58), S( 74,-22), S(-76, 44), S(-45, 50) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -68, -50), S( -22, -30), S( -26, -13), S( -20,   8) },
        { S( -28,  -8), S( -27,   8), S( -10,  -9), S(  -6,   7) },
        { S( -22, -25), S(  -4,   2), S(  -4,  10), S(   4,  36) },
        { S(  -7,  17), S(  13,  31), S(   7,  44), S(   8,  53) },
        { S(   5,  29), S(   1,  35), S(  22,  47), S(   6,  68) },
        { S( -15,  18), S(  11,  32), S(  29,  49), S(  23,  52) },
        { S(  -4,  -4), S( -15,  21), S(  36,  13), S(  50,  26) },
        { S(-178, -48), S(-123,  32), S(-118,  60), S(   6,  18) }
    },

    // Bishop
    {
        { S(   3,  -1), S(  -2,   7), S( -19,   9), S( -25,  22) },
        { S(   2,  -4), S(   4,   0), S(  -3,   9), S( -12,  22) },
        { S(  -4,  15), S(   4,  21), S(  -2,  34), S(  -5,  43) },
        { S(  -2,   6), S(  -1,  30), S(  -7,  49), S(  13,  55) },
        { S(  -6,  23), S(  -5,  47), S(  19,  39), S(  16,  63) },
        { S(   4,  25), S(   8,  50), S(  25,  40), S(  19,  36) },
        { S( -42,  39), S( -30,  44), S( -19,  45), S( -15,  39) },
        { S( -67,  59), S( -59,  49), S(-158,  66), S(-125,  61) }
    },

    // Rook
    {
        { S( -27,   3), S( -24,  10), S( -22,  15), S( -18,   8) },
        { S( -51,  10), S( -39,   8), S( -32,   7), S( -31,  12) },
        { S( -43,  20), S( -33,  21), S( -44,  30), S( -41,  29) },
        { S( -51,  48), S( -40,  50), S( -37,  53), S( -28,  41) },
        { S( -24,  60), S(  -7,  71), S(  -3,  63), S(  13,  52) },
        { S( -17,  70), S(  22,  57), S(  23,  67), S(  47,  47) },
        { S(  -4,  74), S( -10,  80), S(  27,  66), S(  34,  62) },
        { S(  16,  61), S(  21,  64), S(   4,  68), S(  11,  63) }
    },

    // Queen
    {
        { S(  19, -54), S(  16, -73), S(  18, -82), S(  21, -55) },
        { S(  18, -52), S(  24, -56), S(  24, -48), S(  23, -40) },
        { S(  12, -30), S(  14,  -1), S(   8,  22), S(   3,  23) },
        { S(   6,  21), S(  10,  42), S(  -3,  61), S(   2,  79) },
        { S(  11,  37), S(  -5,  86), S(  -2,  96), S(  -8, 115) },
        { S(   2,  46), S(  20,  63), S(  -0, 120), S(  -5, 124) },
        { S(  -6,  58), S( -47, 104), S(  -2, 117), S( -37, 155) },
        { S(  -9,  74), S( -22,  99), S( -15, 117), S( -25, 113) }
    },

    // King
    {
        { S( 268,  -1), S( 292,  49), S( 226,  78), S( 168,  97) },
        { S( 274,  50), S( 273,  86), S( 244, 106), S( 211, 116) },
        { S( 194,  83), S( 250,  96), S( 232, 121), S( 223, 130) },
        { S( 157,  90), S( 250, 113), S( 246, 128), S( 227, 145) },
        { S( 200, 117), S( 281, 140), S( 266, 151), S( 232, 162) },
        { S( 231, 140), S( 331, 174), S( 314, 170), S( 289, 162) },
        { S( 213,  94), S( 273, 184), S( 307, 165), S( 290, 153) },
        { S( 276,-158), S( 350,  41), S( 321,  68), S( 266,  95) }
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
