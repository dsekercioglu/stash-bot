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
    { S(-30, 18), S(-27, 17), S(-37, 17), S(-20, -4), S(-19, 12), S( 13, 11), S( 19, -5), S( -8,-22) },
    { S(-30, 11), S(-41, 19), S(-25,  9), S(-24,  0), S(-14,  8), S(-16, 11), S(  3,-18), S( -8,-17) },
    { S(-31, 20), S(-31, 12), S(-17, -9), S(-12,-25), S( -7,-18), S( -1,-12), S( -2,-14), S(-16,-11) },
    { S(-23, 39), S(-25, 21), S(-17,  3), S(  2,-25), S( 16,-22), S( 27,-25), S(  7, -7), S(-10,  7) },
    { S(-14, 59), S(-11, 39), S( -3, 17), S(  2,-20), S( 25,-34), S(102,-13), S( 51,  7), S( 14, 24) },
    { S( 69, 18), S( 65, 16), S( 70, -4), S( 80,-59), S(105,-63), S( 53,-27), S(-96, 39), S(-65, 45) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -54, -66), S(  -7, -46), S( -13, -29), S(  -6,  -8) },
        { S( -14, -24), S( -13,  -7), S(   4, -25), S(   8,  -9) },
        { S(  -8, -40), S(  10, -14), S(  10,  -7), S(  18,  20) },
        { S(   7,   2), S(  28,  16), S(  22,  27), S(  23,  37) },
        { S(  19,  13), S(  16,  19), S(  37,  31), S(  21,  53) },
        { S(  -1,   2), S(  23,  16), S(  43,  31), S(  36,  35) },
        { S(   9, -19), S(  -1,   5), S(  48,  -4), S(  63,  10) },
        { S(-163, -64), S(-109,  16), S(-104,  44), S(  20,   2) }
    },

    // Bishop
    {
        { S(  18, -34), S(  13, -25), S(  -3, -23), S( -10, -10) },
        { S(  17, -37), S(  19, -31), S(  12, -22), S(   4,  -9) },
        { S(  11, -17), S(  19, -11), S(  13,   2), S(  10,  11) },
        { S(  14, -27), S(  13,  -1), S(   9,  17), S(  29,  24) },
        { S(   9,  -9), S(  12,  15), S(  34,   7), S(  41,  31) },
        { S(  19,  -7), S(  21,  19), S(  39,   9), S(  42,   4) },
        { S( -26,   8), S( -17,  12), S(  -6,  13), S(  -1,   7) },
        { S( -51,  27), S( -44,  16), S(-142,  34), S(-110,  29) }
    },

    // Rook
    {
        { S( -16, -40), S( -13, -33), S( -10, -28), S(  -6, -35) },
        { S( -39, -33), S( -27, -36), S( -19, -36), S( -19, -31) },
        { S( -31, -23), S( -21, -22), S( -31, -13), S( -28, -14) },
        { S( -39,   5), S( -29,   8), S( -24,  10), S( -15,  -2) },
        { S( -11,  17), S(   5,  28), S(   9,  21), S(  25,   9) },
        { S(  -4,  27), S(  34,  14), S(  34,  25), S(  59,   4) },
        { S(   8,  31), S(   2,  37), S(  39,  24), S(  45,  19) },
        { S(  28,  18), S(  33,  21), S(  16,  25), S(  23,  20) }
    },

    // Queen
    {
        { S(  17, -94), S(  14,-112), S(  16,-123), S(  19, -95) },
        { S(  16, -91), S(  22, -96), S(  21, -88), S(  21, -80) },
        { S(   9, -69), S(  12, -41), S(   6, -18), S(   1, -17) },
        { S(   4, -19), S(   8,   3), S(  -4,  20), S(   1,  38) },
        { S(  10,  -2), S(  -6,  46), S(  -4,  57), S(  -9,  75) },
        { S(   0,   6), S(  18,  23), S(  -3,  80), S(  -7,  84) },
        { S(  -8,  19), S( -48,  65), S(  -4,  76), S( -38, 115) },
        { S( -11,  34), S( -25,  58), S( -17,  76), S( -27,  72) }
    },

    // King
    {
        { S(  14,-104), S(  40, -55), S( -33, -24), S( -87,  -7) },
        { S(  21, -54), S(  20, -18), S( -10,   2), S( -44,  12) },
        { S( -61, -20), S(  -4,  -8), S( -23,  17), S( -32,  26) },
        { S( -98, -13), S(  -6,   9), S( -10,  24), S( -29,  41) },
        { S( -56,  13), S(  25,  36), S(  11,  46), S( -24,  58) },
        { S( -24,  36), S(  75,  70), S(  57,  65), S(  34,  57) },
        { S( -41,  -9), S(  18,  79), S(  52,  60), S(  36,  48) },
        { S(  21,-262), S(  96, -63), S(  67, -35), S(  12,  -8) }
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
