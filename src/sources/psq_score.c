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
        { S( -4, 33), S( -7, 14), S(-28, 15), S(-10, 14), S( -6, 31), S( 39, 25), S( 38,  1), S(  9,-15) },
        { S(  1, 28), S(-11, 18), S( -5,  6), S(  1, -3), S( 13, 12), S( 15,  5), S( 23, -5), S( 17,-16) },
        { S( -2, 25), S( -8, 26), S(  6,-14), S( -2,-21), S(  2,-13), S( 12, -4), S(  0, -2), S( -5, -8) },
        { S(  4, 55), S(  2, 35), S(  8, 11), S( 23,-28), S( 41,-17), S( 45,-31), S( 20, 10), S(  1, 17) },
        { S( 16,115), S(  1, 91), S( 14, 41), S( 40,  3), S( 41,-25), S(140, 23), S( 54, 44), S( 19, 56) },
        { S(133, 67), S( 98, 44), S(105, 18), S(116,-19), S(136,-51), S( 97,-31), S(-85, 49), S(-12, 43) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -70, -87), S( -32, -24), S( -29, -31), S( -23,  -2) },
        { S( -18, -26), S( -38,  -9), S( -15, -12), S( -17,  -8) },
        { S( -45, -51), S(  -9, -18), S( -27,  15), S(   3,  46) },
        { S( -13,  -1), S(  22,  21), S(  -1,  57), S(  -8,  59) },
        { S(  13,  21), S(  13,  26), S(  32,  55), S(   1,  71) },
        { S(  11,   5), S( -17,  31), S( -17,  63), S( -27,  56) },
        { S(  -8, -30), S(  -5,  19), S(  21, -15), S(   2,  42) },
        { S(-188, -74), S(-162, -10), S(-174,  46), S(  -8,  17) }
    },

    // Bishop
    {
        { S(   6, -15), S(   8, -22), S( -22,   4), S( -33,  14) },
        { S(  -2, -31), S(   8,   1), S(   5,  -8), S( -29,  30) },
        { S(  -5,  16), S(   3,   7), S(  -9,  34), S(  -3,  34) },
        { S(  -9,  -1), S( -11,  19), S(  -6,  38), S(  27,  46) },
        { S( -37,  25), S(   7,  36), S(  12,  17), S(  30,  56) },
        { S( -19,  48), S(   2,  29), S(   9,  44), S(  -6,  27) },
        { S( -42,  26), S( -67,  56), S( -39,  38), S( -76,  60) },
        { S(-102,  52), S( -78,  53), S(-212,  58), S(-171,  56) }
    },

    // Rook
    {
        { S( -39,   4), S( -40,   5), S( -24,  10), S( -15,  -6) },
        { S( -94,  -6), S( -44, -31), S( -33, -17), S( -39,  -9) },
        { S( -56, -14), S( -46,   9), S( -50,  10), S( -50,   7) },
        { S( -43,  27), S( -31,  52), S( -44,  51), S( -50,  51) },
        { S(  -6,  56), S(   5,  55), S(  13,  61), S(  37,  44) },
        { S( -20,  79), S(  46,  48), S(  39,  72), S(  49,  50) },
        { S(   7,  74), S( -13,  81), S(  30,  81), S(  34,  87) },
        { S( -16,  85), S(  12,  84), S( -48, 107), S( -25,  95) }
    },

    // Queen
    {
        { S(   0, -80), S(  15, -98), S(  19,-105), S(  18, -67) },
        { S(  19, -67), S(  13, -78), S(  28, -86), S(  17, -50) },
        { S(  17, -51), S(  24, -15), S(  21,   3), S(   8,   6) },
        { S(  13,  20), S(   6,  35), S( -13,  71), S(   1, 100) },
        { S(  25,  40), S(  13,  96), S(  10, 107), S(  -2, 136) },
        { S(  -5,  63), S(  26,  56), S(  16, 126), S( -13, 127) },
        { S(  -6,  56), S( -51,  98), S( -12, 121), S( -50, 190) },
        { S( -15,  56), S( -17,  93), S( -15, 127), S( -38, 140) }
    },

    // King
    {
        { S( 244, -51), S( 263,  47), S( 188,  66), S( 145,  55) },
        { S( 255,  40), S( 250,  94), S( 216, 125), S( 174, 128) },
        { S( 177,  64), S( 279,  99), S( 254, 142), S( 247, 168) },
        { S( 150,  88), S( 297, 134), S( 286, 170), S( 273, 193) },
        { S( 188, 110), S( 315, 164), S( 306, 182), S( 268, 197) },
        { S( 233, 120), S( 336, 186), S( 348, 190), S( 308, 166) },
        { S( 204,  69), S( 301, 192), S( 294, 148), S( 289, 145) },
        { S( 228,-180), S( 358,  74), S( 312,  81), S( 248,  69) }
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
