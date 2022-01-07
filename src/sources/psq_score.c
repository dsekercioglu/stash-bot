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
    { S( -6, 18), S( -3, 16), S(-19, 17), S(  1, 11), S(  1, 14), S( 24, 20), S( 24, 14), S( -6, -9) },
    { S( -2,  9), S(-19, 14), S(-10,  6), S(  0, -3), S(  1, 12), S( -5, 15), S(  9, -1), S( -3, -9) },
    { S( -5, 17), S( -9, 18), S(  3,-14), S( 10,-27), S( 14,-21), S( 11, -6), S( -4,  4), S(-14,  1) },
    { S(  7, 34), S(  6, 19), S(  5, -3), S( 31,-33), S( 37,-22), S( 39,-18), S( 11, 10), S(  1, 17) },
    { S( 13, 64), S( 42, 47), S( 57, 13), S( 60,-19), S( 68,-18), S( 89,  7), S( 58, 37), S( 18, 57) },
    { S( 74, 10), S( 74, 19), S( 86,-11), S(100,-43), S(127,-50), S( 84,-19), S(-38, 54), S(-17, 42) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -52, -53), S( -15, -24), S( -19,  -4), S( -18,  12) },
        { S( -22,   3), S( -27,   7), S( -11,  -5), S(  -1,  13) },
        { S( -20, -17), S(  -7,   9), S(  -2,  18), S(  11,  42) },
        { S(  -4,  16), S(  20,  32), S(  12,  54), S(  11,  60) },
        { S(   6,  17), S(   9,  35), S(  34,  55), S(  15,  69) },
        { S( -25,  13), S(  19,  20), S(  23,  61), S(  29,  56) },
        { S( -23, -15), S( -27,  12), S(  32,   7), S(  55,  21) },
        { S(-183, -57), S(-132,  23), S(-127,  48), S(  -9,   8) }
    },

    // Bishop
    {
        { S(   6,  -8), S(   1,  13), S(  -9,   9), S( -13,  18) },
        { S(   8, -10), S(  13,  -3), S(  10,  13), S(  -9,  22) },
        { S(   2,  11), S(   9,  27), S(   3,  35), S(   2,  45) },
        { S(  -2,  12), S(   5,  29), S(  -3,  53), S(  23,  58) },
        { S( -11,  29), S(   0,  47), S(  19,  44), S(  32,  56) },
        { S(  -8,  40), S(  15,  54), S(  26,  49), S(  27,  35) },
        { S( -65,  35), S( -41,  52), S( -16,  44), S( -26,  35) },
        { S( -80,  53), S( -69,  43), S(-163,  59), S(-134,  61) }
    },

    // Rook
    {
        { S( -29,   7), S( -26,  13), S( -18,  15), S( -15,   9) },
        { S( -91,  19), S( -33,   5), S( -29,   8), S( -31,   9) },
        { S( -43,  16), S( -25,  25), S( -38,  27), S( -37,  24) },
        { S( -44,  39), S( -30,  47), S( -32,  49), S( -34,  44) },
        { S( -17,  56), S(   6,  52), S(  13,  53), S(  30,  45) },
        { S( -10,  72), S(  45,  46), S(  30,  59), S(  55,  43) },
        { S(  -9,  81), S( -10,  82), S(  22,  74), S(  26,  78) },
        { S(  22,  75), S(  16,  77), S(  -0,  82), S(   6,  77) }
    },

    // Queen
    {
        { S(  30, -76), S(  26, -68), S(  23, -71), S(  32, -42) },
        { S(  19, -51), S(  27, -55), S(  32, -58), S(  26, -19) },
        { S(  19, -24), S(  21,  -1), S(  19,  29), S(  11,  20) },
        { S(  12,  22), S(  21,  42), S(   5,  64), S(   0,  92) },
        { S(  15,  52), S(   8,  91), S(   5,  89), S(  -1, 116) },
        { S(   2,  65), S(  13,  64), S(  -2, 109), S(  -2, 114) },
        { S( -12,  57), S( -61, 107), S( -13, 117), S( -51, 156) },
        { S( -18,  66), S( -28,  96), S( -20, 115), S( -21, 123) }
    },

    // King
    {
        { S( 246, -10), S( 280,  50), S( 213,  84), S( 213,  70) },
        { S( 252,  58), S( 253,  97), S( 227, 120), S( 197, 129) },
        { S( 171,  84), S( 241, 103), S( 251, 125), S( 248, 144) },
        { S( 157,  81), S( 271, 117), S( 270, 145), S( 242, 162) },
        { S( 192, 111), S( 289, 150), S( 280, 164), S( 242, 171) },
        { S( 226, 126), S( 333, 169), S( 326, 166), S( 291, 152) },
        { S( 208,  79), S( 268, 175), S( 303, 155), S( 286, 140) },
        { S( 274,-160), S( 346,  37), S( 315,  63), S( 264,  91) }
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
