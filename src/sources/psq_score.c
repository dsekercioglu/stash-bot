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
    { S( -3, 32), S( -5, 30), S(-23, 14), S( -9,  2), S( -9, 24), S( 31, 24), S( 37, 11), S( 11,-20) },
    { S(  3, 26), S(-22, 19), S(-10,  9), S( -3,  3), S(  2, 16), S( -5, 27), S( 24, -8), S( 17,-17) },
    { S( -4, 25), S(-13, 24), S(  7,-13), S(  5,-28), S( 13,-19), S( 16, -6), S(  8, -0), S( -6, -5) },
    { S( 11, 55), S( 11, 25), S(  3, 15), S( 22,-30), S( 26,-20), S( 52,-36), S( 27,  8), S( 17, 14) },
    { S( 13,100), S( 12, 70), S( 21, 47), S( 31,-13), S( 32,-24), S(102,-13), S( 62,  4), S( 48, 38) },
    { S( 83, 81), S(113, 50), S( 85, 23), S( 89,-74), S(120,-76), S( 44,-40), S(-70, 50), S(-21, 83) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -62, -73), S( -28, -33), S( -16, -43), S( -39,  11) },
        { S( -25, -14), S( -30, -10), S( -29,  -5), S( -16,  -6) },
        { S( -41, -32), S( -23,   8), S( -22,  10), S(   0,  42) },
        { S( -14,   0), S(  15,  40), S(  -1,  61), S(   1,  63) },
        { S(   7,  36), S(  10,  34), S(  22,  59), S(   4,  70) },
        { S( -15,  31), S(  17,  23), S(  -4,  79), S(  -2,  62) },
        { S( -43,  -6), S( -42,  23), S(   1,  26), S(   4,  37) },
        { S(-173, -36), S(-118,  48), S(-142,  54), S(  -7,  16) }
    },

    // Bishop
    {
        { S(   3,  -4), S(  13,  -3), S( -19,  12), S( -33,  16) },
        { S(   7, -12), S(   9,  -5), S(   3,  -6), S( -20,  17) },
        { S(  -3,  -2), S(   8,  24), S(  -5,  25), S(  -3,  34) },
        { S(  -0, -12), S( -19,  21), S( -14,  46), S(  14,  63) },
        { S( -35,  21), S(  -9,  38), S(  -7,  40), S(  21,  48) },
        { S( -17,  24), S(  14,  61), S( -33,  46), S( -10,  31) },
        { S( -43,  24), S( -83,  46), S( -50,  45), S( -66,  64) },
        { S( -93,  63), S( -95,  56), S(-229,  58), S(-158,  52) }
    },

    // Rook
    {
        { S( -35, -12), S( -34,  10), S( -23,   2), S( -19,   4) },
        { S( -82,   6), S( -54, -14), S( -36,  -7), S( -23, -13) },
        { S( -42,   5), S( -43,  24), S( -51,  27), S( -54,  25) },
        { S( -38,  31), S( -20,  56), S( -36,  56), S( -46,  53) },
        { S( -18,  72), S(  24,  60), S(  -0,  71), S(  31,  56) },
        { S(   4,  79), S(  42,  59), S(  15,  79), S(  42,  55) },
        { S(   6,  84), S( -18,  89), S(  31,  77), S(  40,  84) },
        { S( -31,  83), S(   2,  76), S( -51,  91), S( -28,  81) }
    },

    // Queen
    {
        { S(  13, -68), S(  23,-102), S(   8,-103), S(  18, -67) },
        { S(   9, -73), S(  14, -78), S(  18, -66), S(  12, -48) },
        { S(  20, -41), S(  12, -20), S(  20,   1), S(   0,  12) },
        { S(  10,  15), S(   9,  46), S(  11,  61), S(  -5, 109) },
        { S(  37,  36), S(   2,  94), S(  17, 114), S(  10, 147) },
        { S(  29,  52), S(  29,  81), S(   4, 145), S(   5, 153) },
        { S(   4,  70), S( -51, 108), S(   4, 146), S( -42, 180) },
        { S( -10,  84), S( -29,  87), S( -25, 114), S( -23, 133) }
    },

    // King
    {
        { S( 238, -36), S( 280,  28), S( 183,  54), S( 163,  51) },
        { S( 251,  38), S( 239,  87), S( 206, 121), S( 184, 122) },
        { S( 174,  65), S( 221, 102), S( 211, 141), S( 247, 158) },
        { S( 142,  97), S( 275, 144), S( 260, 168), S( 238, 192) },
        { S( 217, 129), S( 309, 178), S( 276, 200), S( 249, 197) },
        { S( 268, 154), S( 343, 210), S( 349, 209), S( 309, 174) },
        { S( 224,  93), S( 314, 213), S( 348, 191), S( 326, 173) },
        { S( 283,-153), S( 389,  66), S( 332,  79), S( 315, 126) }
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
