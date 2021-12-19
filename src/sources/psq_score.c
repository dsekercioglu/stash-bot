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
    { S(-12, 26), S( -6, 21), S(-14, 19), S(  4, -4), S(  6, 12), S( 30, 17), S( 38,  4), S( 10,-19) },
    { S(-12, 20), S(-22, 20), S( -6, 10), S(  4, -2), S(  6, 13), S(  4, 12), S( 23, -7), S( 10,-13) },
    { S(-14, 27), S(-13, 22), S(  4, -9), S( 12,-25), S( 16,-18), S( 16, -7), S( 12, -4), S( -2, -2) },
    { S( -6, 46), S( -5, 28), S(  2,  7), S( 27,-26), S( 34,-14), S( 41,-19), S( 22,  2), S(  9, 16) },
    { S( -4, 70), S(  7, 54), S( 22, 21), S( 29,-17), S( 50,-25), S(119,-14), S( 73, 16), S( 37, 31) },
    { S( 88, 25), S( 90, 18), S( 87, -4), S(101,-52), S(130,-56), S( 77,-23), S(-74, 44), S(-42, 48) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -62, -51), S( -16, -35), S( -22, -14), S( -23,  10) },
        { S( -28, -12), S( -30,   2), S( -15,  -9), S(  -7,   9) },
        { S( -23, -21), S( -13,  11), S(  -6,  11), S(   3,  38) },
        { S(  -8,  17), S(  15,  31), S(   4,  52), S(   3,  54) },
        { S(   6,  28), S(   3,  33), S(  27,  47), S(   7,  65) },
        { S( -11,  18), S(  16,  30), S(  21,  57), S(  33,  50) },
        { S( -10,  -8), S( -16,  20), S(  36,   8), S(  55,  25) },
        { S(-179, -51), S(-123,  32), S(-119,  58), S(   5,  18) }
    },

    // Bishop
    {
        { S(  -2,   3), S(   5,   9), S( -12,  10), S( -20,  23) },
        { S(   2,  -7), S(   7,   3), S(   6,  11), S( -12,  21) },
        { S(  -3,  16), S(   3,  26), S(  -1,  32), S(  -2,  43) },
        { S(  -3,   8), S(   1,  28), S(  -7,  48), S(  16,  55) },
        { S( -16,  26), S(  -5,  46), S(  19,  39), S(  20,  58) },
        { S(  -8,  28), S(  10,  50), S(  25,  41), S(  17,  34) },
        { S( -49,  34), S( -34,  43), S( -21,  44), S( -14,  40) },
        { S( -69,  58), S( -59,  48), S(-157,  69), S(-129,  62) }
    },

    // Rook
    {
        { S( -30,   3), S( -27,   9), S( -21,  11), S( -18,   8) },
        { S( -61,  11), S( -38,   6), S( -29,   7), S( -34,  10) },
        { S( -43,  22), S( -30,  25), S( -45,  30), S( -43,  29) },
        { S( -46,  46), S( -40,  52), S( -37,  49), S( -37,  45) },
        { S( -31,  64), S(  -5,  64), S(  -1,  63), S(  12,  53) },
        { S( -16,  73), S(  26,  59), S(  27,  61), S(  47,  44) },
        { S(  -3,  72), S(  -9,  76), S(  29,  65), S(  45,  63) },
        { S(  14,  71), S(  23,  65), S(   3,  72), S(  10,  65) }
    },

    // Queen
    {
        { S(  18, -54), S(  15, -77), S(  14, -82), S(  24, -58) },
        { S(  16, -56), S(  19, -60), S(  24, -51), S(  21, -32) },
        { S(  14, -29), S(  15,  -4), S(  11,  25), S(   4,  23) },
        { S(   6,  18), S(  12,  42), S(  -4,  61), S(  -4,  84) },
        { S(  14,  38), S(  -4,  87), S(   1,  95), S(  -8, 115) },
        { S(   8,  50), S(  26,  63), S(   0, 120), S(  -7, 122) },
        { S(  -6,  57), S( -46, 100), S(  -8, 115), S( -40, 156) },
        { S( -10,  76), S( -19, 103), S( -15, 118), S( -19, 117) }
    },

    // King
    {
        { S( 259,  -6), S( 289,  40), S( 222,  72), S( 169,  88) },
        { S( 268,  43), S( 266,  81), S( 237, 105), S( 206, 115) },
        { S( 195,  70), S( 251,  90), S( 228, 119), S( 228, 133) },
        { S( 158,  88), S( 252, 113), S( 251, 135), S( 230, 148) },
        { S( 199, 117), S( 287, 150), S( 272, 156), S( 234, 167) },
        { S( 232, 140), S( 336, 178), S( 326, 179), S( 292, 168) },
        { S( 211,  89), S( 272, 191), S( 308, 176), S( 292, 160) },
        { S( 274,-159), S( 346,  39), S( 317,  66), S( 265,  95) }
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
