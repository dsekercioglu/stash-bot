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
    { S(-10, 23), S( -6, 23), S(-16, 21), S(  0,  1), S(  1, 16), S( 32, 17), S( 41,  1), S( 10,-16) },
    { S(-11, 16), S(-21, 25), S( -4, 12), S( -3,  5), S(  5, 14), S(  3, 15), S( 23,-12), S( 11,-12) },
    { S(-12, 26), S( -9, 16), S(  4, -7), S(  9,-19), S( 12,-14), S( 19, -7), S( 18,-10), S(  4, -6) },
    { S( -3, 44), S( -4, 26), S(  4,  8), S( 21,-21), S( 35,-14), S( 48,-19), S( 26, -3), S( 10, 13) },
    { S(  4, 62), S(  8, 43), S( 17, 21), S( 22,-16), S( 45,-29), S(121, -8), S( 71, 11), S( 34, 29) },
    { S( 90, 23), S( 84, 21), S( 90,  1), S(100,-54), S(125,-59), S( 73,-22), S(-76, 44), S(-45, 50) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -68, -50), S( -19, -30), S( -28, -14), S( -20,   9) },
        { S( -26,  -7), S( -26,  11), S(  -9,  -8), S(  -7,   5) },
        { S( -22, -23), S(  -5,   3), S(  -5,   9), S(   4,  35) },
        { S(  -7,  18), S(  11,  31), S(   7,  45), S(   9,  51) },
        { S(   6,  29), S(   1,  35), S(  23,  46), S(   8,  68) },
        { S( -15,  18), S(   9,  31), S(  29,  46), S(  21,  50) },
        { S(  -5,  -3), S( -15,  22), S(  34,  13), S(  48,  26) },
        { S(-177, -48), S(-123,  32), S(-118,  60), S(   6,  18) }
    },

    // Bishop
    {
        { S(   4,  -1), S(  -2,   7), S( -18,   8), S( -25,  21) },
        { S(   2,  -5), S(   4,   1), S(   1,  10), S( -12,  24) },
        { S(  -5,  16), S(   5,  21), S(  -3,  35), S(  -5,  44) },
        { S(  -2,   5), S(  -0,  31), S(  -7,  49), S(  14,  55) },
        { S(  -6,  23), S(  -4,  45), S(  18,  39), S(  16,  63) },
        { S(   4,  26), S(   7,  51), S(  22,  41), S(  17,  37) },
        { S( -41,  40), S( -32,  44), S( -21,  45), S( -16,  39) },
        { S( -66,  59), S( -59,  48), S(-156,  67), S(-124,  61) }
    },

    // Rook
    {
        { S( -27,   1), S( -24,   8), S( -21,  15), S( -17,   9) },
        { S( -53,   9), S( -40,   7), S( -32,   6), S( -30,  13) },
        { S( -44,  20), S( -31,  22), S( -43,  29), S( -40,  28) },
        { S( -51,  48), S( -40,  52), S( -37,  52), S( -27,  42) },
        { S( -25,  59), S(  -8,  71), S(  -2,  64), S(  13,  53) },
        { S( -16,  71), S(  21,  56), S(  22,  67), S(  47,  48) },
        { S(  -5,  74), S( -10,  80), S(  27,  68), S(  34,  62) },
        { S(  16,  61), S(  21,  64), S(   4,  68), S(  11,  62) }
    },

    // Queen
    {
        { S(  18, -54), S(  13, -72), S(  17, -82), S(  22, -55) },
        { S(  18, -51), S(  23, -56), S(  23, -47), S(  22, -40) },
        { S(  12, -29), S(  15,  -2), S(   9,  23), S(   4,  24) },
        { S(   7,  22), S(   9,  43), S(  -0,  61), S(   1,  78) },
        { S(  14,  38), S(  -4,  86), S(  -0,  97), S(  -6, 114) },
        { S(   2,  46), S(  20,  64), S(  -1, 120), S(  -6, 124) },
        { S(  -7,  59), S( -45, 105), S(  -1, 116), S( -36, 154) },
        { S( -10,  73), S( -23,  98), S( -15, 115), S( -26, 111) }
    },

    // King
    {
        { S( 269,   1), S( 297,  49), S( 222,  81), S( 169,  98) },
        { S( 276,  50), S( 276,  86), S( 245, 107), S( 212, 115) },
        { S( 193,  83), S( 250,  96), S( 232, 120), S( 223, 129) },
        { S( 157,  91), S( 249, 114), S( 244, 128), S( 225, 145) },
        { S( 199, 117), S( 279, 140), S( 265, 150), S( 230, 162) },
        { S( 231, 140), S( 329, 173), S( 310, 169), S( 288, 161) },
        { S( 214,  96), S( 273, 182), S( 307, 163), S( 291, 151) },
        { S( 276,-158), S( 352,  41), S( 323,  69), S( 267,  96) }
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
