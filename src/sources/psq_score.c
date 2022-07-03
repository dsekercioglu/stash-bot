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
    { S(-19, 23), S(  2, 17), S(  5, 21), S( -1, 16), S(  3, 17), S(  2, 18), S( 14,  7), S(  0,-10) },
    { S(-22, 16), S( -5, 13), S(  3,  7), S(  0, 11), S(  9,  9), S( -3,  4), S(  6, -3), S( -6,-13) },
    { S(-13, 20), S( -9, 12), S( 11, -4), S( 15,-18), S( 18,-20), S( 17, -8), S(  5, -3), S( -5, -2) },
    { S(  3, 40), S( 13, 19), S( 25,  6), S( 47,-17), S( 46,-15), S( 34, -8), S( 11,  1), S( 14,  8) },
    { S(  5, 58), S( 23, 47), S( 31, 22), S( 34,-10), S( 51,-19), S( 96,-15), S( 76, 12), S( 29, 28) },
    { S( 91, 22), S( 83, 20), S( 89, -1), S(101,-52), S(124,-56), S( 74,-20), S(-71, 45), S(-42, 53) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -35, -44), S( -24, -34), S( -17, -16), S( -14,   9) },
        { S( -31,  -8), S( -24,  11), S( -10, -10), S(   1,   2) },
        { S( -20, -25), S( -12,   1), S(  -3,  17), S(  -2,  34) },
        { S(  -4,  22), S(   7,  34), S(  15,  43), S(  13,  49) },
        { S(  -9,  25), S(  13,  37), S(  10,  52), S(  16,  67) },
        { S( -25,  18), S(   9,  32), S(  25,  44), S(  23,  50) },
        { S( -21,  -3), S( -21,  22), S(  30,   8), S(  44,  28) },
        { S(-175, -45), S(-121,  35), S(-119,  60), S(   5,  20) }
    },

    // Bishop
    {
        { S(   2, -12), S(  -6,   7), S( -16,  13), S( -10,  18) },
        { S(   0,  -3), S(   1,  -7), S(  -0,  14), S( -11,  23) },
        { S(  -1,  10), S(   1,  27), S(   5,  32), S(  -1,  41) },
        { S( -12,   6), S(   6,  29), S(   3,  52), S(  12,  60) },
        { S( -14,  20), S(  -1,  48), S(   9,  45), S(  23,  70) },
        { S( -17,  22), S(   7,  49), S(  29,  47), S(  21,  37) },
        { S( -44,  40), S( -34,  46), S( -21,  42), S( -18,  39) },
        { S( -67,  57), S( -60,  47), S(-155,  67), S(-123,  62) }
    },

    // Rook
    {
        { S( -33,   7), S( -23,  11), S( -11,   3), S(  -9,   4) },
        { S( -68,   5), S( -38,  10), S( -27,   7), S( -33,  12) },
        { S( -45,  17), S( -42,  25), S( -41,  31), S( -42,  33) },
        { S( -43,  48), S( -44,  55), S( -40,  52), S( -28,  45) },
        { S( -19,  59), S(  -2,  64), S(  -6,  62), S(  14,  51) },
        { S( -15,  70), S(  20,  58), S(  24,  69), S(  46,  48) },
        { S(  -9,  74), S(  -7,  84), S(  30,  68), S(  35,  67) },
        { S(  16,  66), S(  18,  59), S(   5,  68), S(  11,  63) }
    },

    // Queen
    {
        { S(   3, -58), S(  13, -71), S(  19, -81), S(  21, -53) },
        { S(   9, -51), S(  18, -57), S(  27, -49), S(  27, -39) },
        { S(   3, -27), S(  12,  -3), S(  13,  21), S(  10,  26) },
        { S(   7,  26), S(  11,  44), S(  -1,  62), S(   1,  78) },
        { S(  10,  37), S(  -0,  85), S(   4,  97), S(  -0, 114) },
        { S(  -2,  45), S(  22,  66), S(   5, 120), S(  -2, 124) },
        { S( -11,  58), S( -44, 106), S(  -2, 114), S( -34, 154) },
        { S( -12,  72), S( -26,  97), S( -14, 115), S( -27, 112) }
    },

    // King
    {
        { S( 253,  -2), S( 281,  56), S( 230,  86), S( 210,  97) },
        { S( 276,  54), S( 274,  83), S( 245, 104), S( 221, 117) },
        { S( 191,  80), S( 246,  93), S( 231, 115), S( 222, 128) },
        { S( 155,  91), S( 247, 113), S( 241, 128), S( 225, 145) },
        { S( 197, 115), S( 279, 143), S( 264, 150), S( 230, 162) },
        { S( 230, 139), S( 324, 169), S( 305, 170), S( 287, 163) },
        { S( 215,  97), S( 274, 181), S( 305, 162), S( 293, 152) },
        { S( 277,-157), S( 354,  43), S( 326,  72), S( 269,  97) }
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
