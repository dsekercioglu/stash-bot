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
        { S( -9, 25), S( -7, 12), S(-26, 20), S( -7, 14), S( -2, 34), S( 39, 28), S( 37, -1), S(  8,-20) },
        { S( -4, 27), S(-11, 13), S( -1,  7), S( -6, -7), S( 11,  3), S( 21,  5), S( 22, -1), S( 15,-13) },
        { S( -8, 27), S( -9, 25), S(  4,-13), S( -4,-21), S(  7,-14), S( 15, -3), S( 10,  4), S( -6,-12) },
        { S( -5, 52), S(  6, 32), S(  6, 13), S( 21,-21), S( 42,-15), S( 47,-30), S( 22,  9), S( -1, 14) },
        { S( 12,116), S(  4, 90), S( 21, 48), S( 31,  7), S( 36,-27), S(149, 22), S( 52, 41), S( 22, 47) },
        { S(126, 63), S( 87, 34), S( 98, 16), S(127,-30), S(137,-60), S( 84,-33), S(-69, 37), S(-22, 29) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -64, -77), S( -28, -38), S( -22, -22), S( -17,   5) },
        { S( -22, -22), S( -24,  -8), S( -14,  -9), S(  -9,  -4) },
        { S( -36, -42), S(  -5, -12), S( -16,  13), S(   8,  45) },
        { S(  -4,   6), S(  21,  17), S(  -0,  53), S(  -8,  59) },
        { S(  -0,  18), S(  16,  36), S(  33,  55), S(   8,  74) },
        { S(   1,  13), S( -13,  25), S(  -4,  68), S(  -9,  53) },
        { S(   0, -20), S( -12,  -0), S(  26, -18), S(  25,  32) },
        { S(-195, -62), S(-150,   4), S(-155,  43), S(  -1,  25) }
    },

    // Bishop
    {
        { S(   3, -15), S(   7,  -2), S( -20,   1), S( -30,  17) },
        { S(  -2, -24), S(   9,   7), S(   4,  -1), S( -23,  28) },
        { S( -10,  16), S(   8,   8), S(  -3,  30), S(  -0,  34) },
        { S( -15,  -4), S( -11,  15), S(  -1,  39), S(  27,  44) },
        { S( -34,  23), S(  12,  40), S(  14,  26), S(  34,  56) },
        { S( -13,  39), S(   0,  33), S(  14,  51), S(   7,  20) },
        { S( -49,  29), S( -45,  55), S( -18,  45), S( -54,  55) },
        { S( -93,  51), S( -69,  58), S(-187,  55), S(-151,  64) }
    },

    // Rook
    {
        { S( -35,   4), S( -34,  10), S( -20,  11), S( -10,  -2) },
        { S( -85,  -0), S( -37, -19), S( -31, -11), S( -38,  -3) },
        { S( -55,  -4), S( -48,   7), S( -52,  13), S( -50,   9) },
        { S( -42,  36), S( -32,  55), S( -42,  53), S( -41,  51) },
        { S(  -8,  54), S(   5,  61), S(  12,  62), S(  35,  49) },
        { S( -19,  76), S(  48,  48), S(  29,  70), S(  55,  51) },
        { S(  11,  72), S( -15,  76), S(  33,  76), S(  32,  88) },
        { S(   7,  77), S(  21,  87), S( -29, 101), S( -16,  90) }
    },

    // Queen
    {
        { S(  17, -67), S(  16, -96), S(  17, -86), S(  21, -58) },
        { S(  23, -72), S(  17, -80), S(  29, -76), S(  17, -41) },
        { S(  19, -48), S(  30,  -9), S(  19,   7), S(  10,   6) },
        { S(  12,  15), S(   2,  37), S( -13,  72), S(  -9,  97) },
        { S(  22,  42), S(  15,  92), S(   8,  97), S(   4, 134) },
        { S(  -3,  59), S(  18,  54), S(   9, 124), S( -10, 123) },
        { S(  -1,  59), S( -53,  94), S( -12, 118), S( -48, 186) },
        { S( -14,  65), S(  -8, 108), S( -13, 121), S( -32, 132) }
    },

    // King
    {
        { S( 248, -42), S( 262,  52), S( 192,  72), S( 146,  58) },
        { S( 258,  45), S( 245,  98), S( 209, 122), S( 174, 129) },
        { S( 181,  66), S( 274, 104), S( 255, 138), S( 255, 161) },
        { S( 159,  87), S( 294, 137), S( 286, 162), S( 280, 185) },
        { S( 192, 112), S( 310, 160), S( 297, 173), S( 281, 188) },
        { S( 237, 126), S( 353, 185), S( 352, 185), S( 302, 162) },
        { S( 187,  64), S( 281, 191), S( 283, 137), S( 277, 141) },
        { S( 219,-165), S( 323,  46), S( 295,  68), S( 227,  74) }
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
