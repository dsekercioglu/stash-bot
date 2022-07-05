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
    { S(-10, 26), S( -5, 21), S(-17, 20), S( -2,  4), S(  1, 17), S( 32, 18), S( 38,  3), S(  9,-16) },
    { S( -9, 17), S(-19, 24), S( -3, 11), S( -2,  5), S(  7, 11), S(  1, 16), S( 22,-11), S( 11,-13) },
    { S(-11, 25), S( -8, 17), S(  4, -8), S(  8,-19), S( 13,-15), S( 18, -7), S( 17, -8), S(  6, -7) },
    { S( -2, 44), S( -3, 28), S(  2,  8), S( 22,-23), S( 36,-15), S( 47,-18), S( 23,  3), S( 11, 12) },
    { S(  5, 65), S(  8, 43), S( 19, 20), S( 26,-16), S( 45,-28), S(117,-10), S( 71, 11), S( 38, 28) },
    { S( 91, 24), S( 84, 21), S( 90,  0), S(101,-54), S(125,-56), S( 73,-20), S(-76, 43), S(-46, 50) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -68, -48), S( -18, -32), S( -27, -12), S( -21,  12) },
        { S( -23,  -8), S( -26,  13), S( -13,  -9), S(  -6,   3) },
        { S( -21, -25), S(  -5,   1), S(  -4,   7), S(   5,  32) },
        { S(  -5,  19), S(  12,  32), S(   8,  45), S(   9,  53) },
        { S(   5,  31), S(   2,  35), S(  23,  47), S(   6,  69) },
        { S( -15,  19), S(   6,  30), S(  29,  43), S(  23,  52) },
        { S( -10,  -1), S( -15,  21), S(  32,  11), S(  47,  27) },
        { S(-175, -45), S(-123,  33), S(-117,  61), S(   6,  19) }
    },

    // Bishop
    {
        { S(   4,  -4), S(  -1,   6), S( -16,   8), S( -23,  23) },
        { S(   1,  -3), S(   4,   2), S(   1,   9), S( -12,  25) },
        { S(  -4,  16), S(   7,  22), S(  -2,  35), S(  -3,  43) },
        { S(  -2,   4), S(  -2,  31), S(  -5,  51), S(  13,  58) },
        { S( -12,  22), S(  -3,  46), S(  17,  39), S(  16,  63) },
        { S(   1,  25), S(   7,  48), S(  20,  39), S(  18,  37) },
        { S( -41,  41), S( -35,  43), S( -21,  43), S( -18,  40) },
        { S( -64,  59), S( -59,  48), S(-155,  67), S(-122,  62) }
    },

    // Rook
    {
        { S( -27,   3), S( -24,  10), S( -22,  16), S( -18,   8) },
        { S( -53,   9), S( -41,   8), S( -30,   8), S( -30,  13) },
        { S( -43,  19), S( -32,  24), S( -41,  29), S( -41,  28) },
        { S( -50,  45), S( -41,  52), S( -37,  52), S( -26,  40) },
        { S( -25,  60), S(  -7,  71), S(  -1,  64), S(  16,  55) },
        { S( -16,  69), S(  20,  58), S(  21,  66), S(  45,  46) },
        { S(  -3,  76), S(  -9,  80), S(  27,  69), S(  34,  65) },
        { S(  16,  62), S(  19,  63), S(   6,  69), S(  11,  61) }
    },

    // Queen
    {
        { S(  18, -52), S(  13, -70), S(  14, -81), S(  22, -53) },
        { S(  13, -50), S(  22, -56), S(  23, -47), S(  22, -38) },
        { S(  14, -26), S(  15,  -1), S(   9,  24), S(   4,  21) },
        { S(   5,  23), S(  11,  43), S(  -2,  60), S(  -0,  78) },
        { S(  13,  38), S(  -3,  85), S(   2,  96), S(  -7, 112) },
        { S(   5,  46), S(  20,  65), S(  -0, 119), S(  -2, 124) },
        { S(  -7,  60), S( -47, 105), S(  -2, 114), S( -36, 153) },
        { S( -10,  73), S( -24,  96), S( -15, 115), S( -26, 110) }
    },

    // King
    {
        { S( 266,   2), S( 292,  51), S( 223,  77), S( 190,  90) },
        { S( 272,  50), S( 277,  82), S( 243, 105), S( 213, 113) },
        { S( 192,  83), S( 250,  93), S( 231, 118), S( 223, 129) },
        { S( 157,  92), S( 248, 113), S( 242, 130), S( 225, 146) },
        { S( 198, 116), S( 279, 142), S( 264, 154), S( 230, 166) },
        { S( 231, 141), S( 326, 173), S( 306, 169), S( 287, 160) },
        { S( 215,  97), S( 274, 181), S( 306, 164), S( 292, 151) },
        { S( 277,-157), S( 354,  44), S( 325,  72), S( 269,  98) }
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
