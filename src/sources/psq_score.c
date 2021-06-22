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
    { S(-10, 27), S( -5, 17), S(-24, 25), S(-18, -9), S( -7, 35), S( 35, 23), S( 41,  6), S( 10,-24) },
    { S( -4, 12), S(-12, 26), S(  2,  6), S( -2,  6), S( 13,  6), S( 14, 12), S( 35, -1), S( 14,-12) },
    { S( -5, 23), S(-11, 20), S(  4,-10), S(  1,-26), S( -2,-22), S( 13, -3), S( -4,  3), S( -5,  2) },
    { S(-11, 44), S(  2, 42), S( -5,  6), S( 21,-24), S( 36,-27), S( 48,-22), S( 18, 11), S(  3, 11) },
    { S(  5,114), S( 14, 93), S( 18, 57), S( 42, -8), S( 43,-21), S(132,  9), S( 30, 53), S( 30, 47) },
    { S(133, 58), S(103, 45), S( 76,  7), S(112,-33), S(134,-64), S( 91,-33), S(-56, 49), S(-26, 32) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -71, -74), S( -34, -57), S( -37, -26), S( -32,   1) },
        { S( -35, -32), S( -47, -16), S( -21, -33), S( -14,  13) },
        { S( -39, -41), S( -10,  -4), S( -15,   7), S(   1,  50) },
        { S( -16,   2), S(  33,  19), S(  11,  59), S(  -7,  68) },
        { S(  31,  17), S(   7,  16), S(  29,  58), S(  14,  70) },
        { S( -12,   7), S(  -9,  40), S( -17,  77), S( -23,  61) },
        { S(   2, -24), S( -36, -10), S(  21,  -4), S(   4,  41) },
        { S(-174, -31), S(-130,  30), S(-144,  67), S( -30,   4) }
    },

    // Bishop
    {
        { S(   8, -16), S(  -5,  -7), S( -18,   4), S( -34,  12) },
        { S( -13, -32), S(   4,  -2), S(   8,  -0), S( -22,  22) },
        { S(  -6,  26), S(   7,  17), S(  -6,  28), S(  -2,  32) },
        { S( -24,  -1), S( -14,  24), S(  -3,  43), S(  13,  45) },
        { S( -23,  28), S(  20,  43), S(   6,  30), S(  29,  52) },
        { S(  -5,  45), S(  -1,  42), S(   1,  49), S(  13,  35) },
        { S( -49,  23), S( -64,  47), S( -24,  35), S( -79,  18) },
        { S( -95,  62), S( -86,  42), S(-202,  55), S(-151,  67) }
    },

    // Rook
    {
        { S( -38,  -7), S( -30,  10), S( -18,   3), S( -19,  -2) },
        { S( -93, -13), S( -40, -28), S( -38, -11), S( -42, -16) },
        { S( -59, -11), S( -44,  13), S( -64,  21), S( -41,  17) },
        { S( -58,  37), S( -33,  50), S( -56,  56), S( -34,  44) },
        { S( -21,  56), S(   5,  60), S(  18,  57), S(  50,  57) },
        { S( -14,  74), S(  32,  54), S(  35,  65), S(  49,  46) },
        { S(  -3,  78), S( -13,  85), S(  31,  82), S(  34,  86) },
        { S(  -9,  93), S(  22,  89), S( -51, 106), S( -14, 102) }
    },

    // Queen
    {
        { S(  28, -66), S(  26,-102), S(  13, -74), S(  24, -52) },
        { S(  12, -94), S(  22, -86), S(  28, -73), S(  18, -46) },
        { S(  17, -48), S(  22, -13), S(  15,   9), S(   7,   6) },
        { S(   9,   4), S(  18,  43), S(   3,  64), S(   3, 103) },
        { S(  16,  36), S(  16,  90), S(  10, 102), S( -12, 123) },
        { S(  -9,  62), S(  16,  58), S(  21, 129), S(   5, 136) },
        { S(  -9,  59), S( -59,  99), S( -12, 114), S( -52, 185) },
        { S( -29,  53), S( -24,  98), S( -27, 124), S( -30, 132) }
    },

    // King
    {
        { S( 249, -40), S( 265,  48), S( 184,  75), S( 151,  37) },
        { S( 257,  47), S( 239,  99), S( 203, 126), S( 188, 128) },
        { S( 184,  71), S( 265, 101), S( 247, 141), S( 247, 163) },
        { S( 148,  79), S( 301, 127), S( 291, 171), S( 281, 187) },
        { S( 180, 107), S( 324, 157), S( 302, 189), S( 259, 199) },
        { S( 237, 128), S( 352, 178), S( 345, 188), S( 298, 174) },
        { S( 220,  83), S( 279, 188), S( 308, 141), S( 288, 147) },
        { S( 231,-144), S( 310,   7), S( 274,  60), S( 236,  81) }
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
