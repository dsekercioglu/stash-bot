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
    { S(-10, 22), S(-13,  9), S( -5, 14), S( -5, 14), S( -1, 33), S( 37, 18), S( 36, -0), S(  4,-16) },
    { S( -7, 13), S(-18, 13), S( -7, 13), S( -5, -0), S(  7, 18), S( 11, 12), S( 14, -7), S( 10,-14) },
    { S(-15, 16), S( -6, 14), S( -2, -3), S(  3,-13), S(  9,-13), S( 11,  5), S( 22, -5), S(  8,-13) },
    { S( -9, 43), S( -2, 31), S(  4, 15), S( 19,-19), S( 37,-18), S( 66,-21), S( 27,  0), S( 26, -1) },
    { S( -5, 89), S( 22, 67), S( 35, 39), S( 39, -1), S( 41,-21), S(122,  3), S( 50, 28), S( 43, 34) },
    { S( 73, 48), S( 90, 36), S( 71, -6), S(124,-51), S(128,-62), S( 37,-69), S(-82,-13), S(-29, 29) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -56, -45), S( -14, -22), S( -16, -25), S(  -7,  -6) },
        { S(  -2, -13), S( -24,  -3), S( -15,   3), S(  -9,   7) },
        { S( -20, -14), S(  -7,   6), S( -10,  23), S(   6,  34) },
        { S(   2,  24), S(  27,  32), S(   4,  50), S(   4,  56) },
        { S(  33,  36), S(   9,  40), S(  17,  54), S(  15,  68) },
        { S(  18,  21), S(  11,  44), S(  14,  67), S(  11,  54) },
        { S(   3,   2), S(  -3,  23), S(  33,  17), S(  57,  41) },
        { S(-143,  15), S( -76,  77), S( -87,  70), S(  43,  56) }
    },

    // Bishop
    {
        { S(  19,  -7), S(  22,  22), S( -12,  13), S( -20,  28) },
        { S( -11,   3), S(   2,   5), S(   1,  13), S( -11,  14) },
        { S(   0,  15), S(  -6,  36), S(  -0,  25), S(  -0,  37) },
        { S(   6,  18), S(  -0,  17), S(   1,  42), S(  14,  47) },
        { S( -10,  31), S(   6,  46), S(  23,  29), S(  23,  46) },
        { S(  15,  27), S(  19,  50), S(  27,  52), S(  23,  42) },
        { S( -24,  45), S( -50,  56), S(  -4,  50), S( -23,  51) },
        { S( -41,  62), S( -50,  62), S(-137,  72), S( -82,  79) }
    },

    // Rook
    {
        { S( -36,   5), S( -22,   9), S( -23,   8), S( -18,  11) },
        { S( -70,  24), S( -23,  -2), S( -24,   7), S( -28,   5) },
        { S( -43,  11), S( -26,  18), S( -38,  28), S( -36,  21) },
        { S( -41,  36), S( -22,  48), S( -27,  47), S( -28,  40) },
        { S(  -8,  51), S(   9,  60), S(  13,  54), S(  18,  48) },
        { S( -22,  70), S(  23,  62), S(  32,  62), S(  38,  52) },
        { S(   2,  81), S( -10,  90), S(  13,  73), S(  21,  80) },
        { S(  14,  76), S(  27,  85), S( -13,  92), S(  11,  76) }
    },

    // Queen
    {
        { S(  22, -56), S(  21, -64), S(   9, -53), S(  16, -45) },
        { S(  24, -37), S(  20, -54), S(  20, -43), S(  19, -38) },
        { S(  35, -26), S(  22, -13), S(   8,  30), S(   4,  15) },
        { S(  14,  11), S(  25,  35), S(   1,  52), S(  -5,  63) },
        { S(  42,  18), S(  10,  85), S(  -6, 105), S(  -5,  99) },
        { S(  23,  49), S(  21,  68), S(   8, 111), S(   6, 113) },
        { S(  15,  52), S( -28, 103), S(  -4, 113), S( -31, 155) },
        { S(  -6,  93), S(   8,  85), S( -32, 112), S( -23, 131) }
    },

    // King
    {
        { S( 251, -25), S( 255,  42), S( 223,  51), S( 162,  66) },
        { S( 246,  58), S( 244,  86), S( 196, 112), S( 189, 120) },
        { S( 206,  86), S( 237, 107), S( 208, 133), S( 236, 143) },
        { S( 179, 121), S( 272, 134), S( 245, 159), S( 251, 161) },
        { S( 208, 146), S( 284, 177), S( 321, 187), S( 294, 198) },
        { S( 283, 151), S( 350, 178), S( 343, 204), S( 336, 199) },
        { S( 253, 116), S( 275, 215), S( 300, 193), S( 300, 176) },
        { S( 299,-114), S( 360,  68), S( 291,  84), S( 265, 128) }
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
