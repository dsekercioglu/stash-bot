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
    { S( -6, 33), S( -3, 20), S(-22, 25), S(-15,-14), S(-11, 36), S( 30, 27), S( 39,  4), S(  4,-26) },
    { S(  0, 15), S( -7, 24), S(  3,  7), S(  1,  1), S( 15,  8), S(  8, 10), S( 34, -4), S( 12,-14) },
    { S( -5, 24), S( -8, 25), S(  3, -8), S(  5,-26), S( -7,-20), S( 10, -4), S( -4,  2), S(-12, -2) },
    { S( -4, 48), S(  3, 42), S( -6,  5), S( 24,-25), S( 40,-26), S( 43,-22), S( 14,  9), S(  8,  8) },
    { S( -1,112), S( -1, 89), S( 17, 59), S( 38, -9), S( 32,-27), S(128,  2), S( 13, 45), S( 36, 46) },
    { S(136, 59), S(107, 49), S( 79, 12), S(109,-32), S(133,-54), S( 92,-46), S(-76, 62), S(-39, 45) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -87,-103), S( -34, -53), S( -45, -25), S( -32,   0) },
        { S( -47, -25), S( -32, -15), S( -25, -26), S( -19,  14) },
        { S( -46, -41), S( -15,  -2), S( -18,  13), S(  -3,  51) },
        { S( -17,   9), S(  26,  21), S(   8,  60), S(  -8,  69) },
        { S(  18,  26), S(   3,  19), S(  20,  56), S(   9,  73) },
        { S( -11,  10), S( -19,  36), S( -42,  89), S( -45,  81) },
        { S(   9, -35), S( -51,  -1), S(  16,   3), S( -38,  53) },
        { S(-150, -21), S(-157,  34), S(-155,  53), S( -39,  10) }
    },

    // Bishop
    {
        { S(   2, -19), S( -12,  -1), S( -21,   1), S( -50,  22) },
        { S(  -4, -44), S(   2,  -1), S(   4,  -1), S( -26,  24) },
        { S(  -8,  13), S(   9,   8), S(  -8,  29), S(  -6,  37) },
        { S( -29,   2), S( -21,  29), S( -10,  47), S(  13,  47) },
        { S( -21,  31), S(  15,  44), S(  -0,  25), S(  25,  55) },
        { S( -12,  52), S( -11,  50), S( -14,  59), S(  -7,  28) },
        { S( -54,  23), S( -87,  63), S( -26,  39), S(-119,  28) },
        { S(-124,  72), S(-111,  59), S(-249,  59), S(-185,  79) }
    },

    // Rook
    {
        { S( -42,  -3), S( -34,   7), S( -22,  -0), S( -21,  -6) },
        { S( -94,  -8), S( -33, -37), S( -34, -16), S( -47, -16) },
        { S( -61,  -9), S( -40,  14), S( -74,  25), S( -50,  23) },
        { S( -66,  43), S( -36,  53), S( -68,  60), S( -44,  43) },
        { S( -23,  61), S(   1,  59), S(  19,  50), S(  45,  47) },
        { S( -11,  75), S(  37,  51), S(  34,  63), S(  60,  43) },
        { S(  -9,  86), S( -13,  87), S(  32,  84), S(  27,  86) },
        { S( -51, 109), S(   8,  94), S(-107, 128), S( -41, 113) }
    },

    // Queen
    {
        { S(  21, -93), S(  26,-106), S(   5, -78), S(  21, -63) },
        { S(  16,-109), S(  20,-111), S(  29, -85), S(  21, -59) },
        { S(  13, -48), S(  21, -16), S(  12,  13), S(   7,  -1) },
        { S(   3,  11), S(  16,  44), S(  -3,  67), S(  -7, 107) },
        { S(  11,  35), S(  12,  96), S(  -4, 110), S(  -6, 123) },
        { S(  -2,  64), S(  17,  73), S(  22, 129), S(   0, 147) },
        { S(  -6,  51), S( -57, 107), S(  -8, 122), S( -56, 187) },
        { S( -29,  47), S( -23, 100), S( -21, 131), S( -41, 134) }
    },

    // King
    {
        { S( 252, -46), S( 267,  44), S( 184,  68), S( 152,  29) },
        { S( 260,  45), S( 246,  96), S( 196, 132), S( 184, 127) },
        { S( 173,  66), S( 272, 100), S( 239, 146), S( 239, 169) },
        { S( 125,  73), S( 285, 131), S( 305, 167), S( 275, 196) },
        { S( 188, 111), S( 305, 163), S( 285, 189), S( 244, 201) },
        { S( 223, 123), S( 345, 180), S( 342, 183), S( 295, 171) },
        { S( 215,  91), S( 278, 185), S( 302, 144), S( 278, 143) },
        { S( 228,-151), S( 307,  14), S( 269,  65), S( 239,  84) }
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
