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
    { S( -5, 20), S( -3, 18), S(-19, 18), S( -3, 10), S( -2, 15), S( 30, 19), S( 27, 13), S( -6, -9) },
    { S( -2, 12), S(-18, 16), S( -9,  7), S(  3, -3), S(  4, 13), S( -2, 15), S(  8,  1), S( -3, -8) },
    { S( -4, 20), S( -9, 19), S(  1,-14), S(  7,-27), S( 14,-21), S( 11, -6), S( -3,  2), S(-12,  1) },
    { S(  6, 36), S(  5, 19), S(  5, -2), S( 29,-33), S( 36,-20), S( 38,-20), S( 11,  7), S(  0, 15) },
    { S( 13, 63), S( 40, 45), S( 55, 11), S( 59,-20), S( 67,-19), S( 90,  6), S( 58, 35), S( 19, 54) },
    { S( 74, 10), S( 74, 18), S( 86,-11), S(100,-44), S(127,-51), S( 84,-20), S(-38, 53), S(-17, 41) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -52, -53), S( -16, -24), S( -19,  -4), S( -18,  12) },
        { S( -23,   2), S( -27,   7), S( -11,  -6), S(  -2,  12) },
        { S( -21, -17), S(  -7,  10), S(  -3,  17), S(  10,  42) },
        { S(  -4,  16), S(  20,  32), S(  11,  55), S(  11,  60) },
        { S(   7,  18), S(  10,  35), S(  33,  54), S(  17,  68) },
        { S( -25,  13), S(  19,  21), S(  23,  61), S(  29,  55) },
        { S( -22, -14), S( -27,  13), S(  32,   8), S(  55,  21) },
        { S(-183, -57), S(-132,  23), S(-127,  48), S(  -9,   9) }
    },

    // Bishop
    {
        { S(   5,  -8), S(   1,  13), S(  -8,  10), S( -13,  19) },
        { S(   7, -10), S(  11,  -2), S(   9,  13), S( -11,  22) },
        { S(   3,  12), S(   8,  26), S(   2,  34), S(   3,  44) },
        { S(  -2,  12), S(   6,  30), S(  -1,  52), S(  24,  57) },
        { S( -11,  29), S(   2,  47), S(  19,  44), S(  32,  56) },
        { S(  -8,  39), S(  15,  54), S(  26,  48), S(  26,  35) },
        { S( -64,  35), S( -41,  52), S( -16,  44), S( -25,  36) },
        { S( -80,  53), S( -69,  43), S(-163,  59), S(-134,  61) }
    },

    // Rook
    {
        { S( -28,   6), S( -26,  12), S( -18,  13), S( -16,   8) },
        { S( -89,  20), S( -34,   5), S( -30,   7), S( -32,   8) },
        { S( -42,  17), S( -26,  25), S( -39,  27), S( -38,  24) },
        { S( -43,  40), S( -31,  47), S( -32,  49), S( -34,  45) },
        { S( -17,  58), S(   6,  53), S(  13,  55), S(  30,  46) },
        { S( -10,  73), S(  45,  47), S(  31,  60), S(  55,  43) },
        { S(  -8,  81), S(  -9,  82), S(  22,  74), S(  27,  77) },
        { S(  22,  74), S(  16,  76), S(  -0,  81), S(   6,  76) }
    },

    // Queen
    {
        { S(  30, -76), S(  25, -68), S(  22, -71), S(  31, -43) },
        { S(  19, -51), S(  26, -55), S(  32, -57), S(  25, -21) },
        { S(  19, -24), S(  20,  -1), S(  17,  28), S(  10,  20) },
        { S(  12,  22), S(  20,  42), S(   5,  64), S(   1,  92) },
        { S(  16,  52), S(   8,  91), S(   5,  89), S(  -0, 116) },
        { S(   3,  65), S(  14,  65), S(  -1, 110), S(  -2, 114) },
        { S( -11,  58), S( -60, 108), S( -13, 117), S( -50, 156) },
        { S( -18,  66), S( -28,  96), S( -20, 115), S( -21, 123) }
    },

    // King
    {
        { S( 248,  -7), S( 279,  50), S( 213,  84), S( 211,  72) },
        { S( 255,  58), S( 254,  97), S( 227, 119), S( 197, 129) },
        { S( 172,  85), S( 241, 103), S( 250, 126), S( 247, 143) },
        { S( 157,  82), S( 270, 116), S( 269, 143), S( 241, 160) },
        { S( 192, 112), S( 289, 149), S( 279, 162), S( 241, 171) },
        { S( 226, 126), S( 333, 169), S( 326, 166), S( 291, 153) },
        { S( 208,  79), S( 268, 175), S( 303, 155), S( 287, 140) },
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
