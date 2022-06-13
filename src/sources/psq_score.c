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

#include "psq_score.h"
#include "types.h"

// clang-format off

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
    { S(-11, 18), S( -9, 16), S(-17, 11), S( -0,  3), S(  2, 14), S( 32, 23), S( 38, 12), S( 10,-14) },
    { S(-10, 10), S(-22, 14), S( -2,  7), S( -1,  2), S(  7, 11), S(  3, 14), S( 23, -5), S( 14,-11) },
    { S(-11, 18), S(-12, 10), S(  6, -9), S( 11,-17), S( 15,-11), S( 18, -1), S( 17, -6), S(  4, -6) },
    { S( -4, 35), S( -4, 20), S(  2,  7), S( 25,-16), S( 38, -5), S( 52, -5), S( 19,  7), S(  9, 12) },
    { S(  4, 51), S(  6, 38), S( 19, 16), S( 28,-18), S( 48,-20), S(123, 11), S( 73, 17), S( 37, 27) },
    { S( 92, 24), S( 79, 18), S( 91,  1), S(103,-42), S(125,-51), S( 71,-20), S(-77, 33), S(-50, 37) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -68, -46), S( -17, -29), S( -27, -14), S( -20,   5) },
        { S( -22,  -8), S( -29,  10), S( -12,  -9), S(  -6,   5) },
        { S( -22, -20), S(  -7,   1), S(  -5,   7), S(   6,  30) },
        { S(  -6,  22), S(  10,  30), S(   8,  41), S(   9,  47) },
        { S(   6,  37), S(   3,  34), S(  25,  45), S(   8,  62) },
        { S( -13,  21), S(   7,  28), S(  34,  45), S(  27,  52) },
        { S( -11,   1), S( -16,  18), S(  35,  19), S(  47,  37) },
        { S(-177, -45), S(-123,  29), S(-117,  51), S(   7,  24) }
    },

    // Bishop
    {
        { S(   7,   4), S(  -4,  10), S( -17,  10), S( -25,  20) },
        { S(   3,   4), S(   4,   9), S(   3,  14), S( -11,  24) },
        { S(  -3,  17), S(   7,  29), S(  -2,  36), S(  -4,  41) },
        { S(   1,   7), S(  -0,  32), S(  -5,  49), S(  16,  58) },
        { S(  -9,  21), S(  -4,  47), S(  17,  45), S(  17,  67) },
        { S(   3,  26), S(   6,  48), S(  21,  47), S(  22,  43) },
        { S( -42,  35), S( -40,  37), S( -23,  39), S( -18,  38) },
        { S( -64,  53), S( -59,  45), S(-155,  47), S(-121,  44) }
    },

    // Rook
    {
        { S( -27,   5), S( -23,   9), S( -20,  15), S( -16,  11) },
        { S( -54,   6), S( -41,   3), S( -29,   8), S( -29,  11) },
        { S( -44,  13), S( -34,  20), S( -43,  22), S( -42,  23) },
        { S( -55,  35), S( -40,  43), S( -39,  43), S( -26,  37) },
        { S( -22,  55), S(  -7,  67), S(   4,  61), S(  19,  59) },
        { S( -12,  63), S(  17,  64), S(  23,  71), S(  48,  62) },
        { S(  -5,  71), S( -13,  73), S(  26,  72), S(  32,  74) },
        { S(  16,  66), S(  18,  65), S(   5,  67), S(   8,  62) }
    },

    // Queen
    {
        { S(  21, -43), S(  17, -60), S(  19, -67), S(  25, -36) },
        { S(  19, -44), S(  27, -43), S(  26, -31), S(  26, -26) },
        { S(  15, -19), S(  17,   5), S(  12,  23), S(   6,  23) },
        { S(   8,  26), S(  12,  43), S(  -4,  56), S(   1,  73) },
        { S(  16,  38), S(  -4,  76), S(  -3,  93), S(  -5, 106) },
        { S(  -0,  42), S(  16,  65), S(  -4, 114), S(  -7, 120) },
        { S(  -8,  60), S( -52,  95), S(  -3, 107), S( -43, 138) },
        { S( -13,  72), S( -26,  90), S( -16, 110), S( -27, 107) }
    },

    // King
    {
        { S( 278,  28), S( 307,  70), S( 228,  74), S( 169,  81) },
        { S( 285,  70), S( 288,  93), S( 247, 104), S( 211, 103) },
        { S( 184,  76), S( 248,  96), S( 224, 112), S( 216, 119) },
        { S( 153,  80), S( 246, 111), S( 240, 126), S( 223, 135) },
        { S( 197, 108), S( 277, 146), S( 265, 152), S( 230, 155) },
        { S( 231, 137), S( 327, 172), S( 308, 173), S( 287, 164) },
        { S( 215,  99), S( 274, 177), S( 306, 162), S( 292, 149) },
        { S( 277,-155), S( 353,  48), S( 324,  79), S( 268, 100) }
    }
};

#undef S

// clang-format on

void psq_score_init(void)
{
    for (piece_t piece = WHITE_PAWN; piece <= WHITE_KING; ++piece)
    {
        scorepair_t pieceValue =
            create_scorepair(PieceScores[MIDGAME][piece], PieceScores[ENDGAME][piece]);

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
