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

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB] = {
    { },

    // Pawn
    {        
        { },
        { S(-17, 30), S(-44, 27), S(-55, 34), S(-27,-17), S(-23,  6), S( 28, 28), S( 24, 16), S(-17, -8) },
        { S(-14, 14), S(-22,  4), S(-39,  5), S(-32, -7), S(-15,  4), S( 28,  9), S( 32, -9), S( 19,-13) },
        { S( -2, 20), S(-17, 13), S(  0,-19), S( -8,-46), S(  7,-30), S( 44, -4), S( 32,  1), S( 18,-12) },
        { S( 15, 51), S( 21, 25), S( 11,  9), S( 45,-40), S( 72,-28), S(103,-11), S( 51, 18), S( 30, 17) },
        { S( 35, 91), S( 56, 71), S( 54, 32), S( 85,-10), S(100,-24), S(155, 16), S(111, 27), S( 45, 39) },
        { S( 99, 19), S( 79, 16), S( 92, -9), S(111,-53), S( 85,-62), S( 31,-50), S(-52,-15), S(-55,-11) },
        { }
    },
    
    // Knight
    {
        { S(-120, -76), S( -35, -79), S( -54, -53), S( -20, -31), S(  -8, -29), S(  -9, -66), S( -26, -62), S( -86, -64) },
        { S( -70, -66), S( -43, -25), S( -29, -48), S( -13, -28), S(  -7, -31), S(  12, -40), S( -25, -17), S(   1, -33) },
        { S( -56, -69), S( -42, -32), S( -36, -14), S(  15,  19), S(  35,  10), S( -14, -37), S(  19, -21), S(  -1, -31) },
        { S( -33, -24), S(  17,  -4), S(   2,  14), S(  -8,  25), S(  18,  33), S(  50,  30), S(  80,  10), S(  42,   5) },
        { S(  -6,  -5), S( -18,  -4), S(   0,  26), S(  24,  37), S(   7,  36), S(  73,  38), S(  41,  30), S(  88,  17) },
        { S(   2,  -6), S( -21,  14), S( -41,  38), S(  17,  23), S(  37,  32), S(  35,  46), S(  47,  17), S(  45,  17) },
        { S(   9,   0), S(   6,  30), S(  45,  17), S(  -6,  51), S(  58,  41), S(  55,  -3), S(  28,  17), S(  58,   8) },
        { S(-140, -83), S( -16,  -3), S( -75,  27), S(  -3,  11), S(   4,  26), S( -55,  31), S(  24,   1), S(-133, -97) }
    },

    // Bishop
    {
        { S(   0, -29), S(  58, -27), S( -20, -20), S( -49,  -3), S( -39, -15), S( -25,  -4), S(   8, -19), S( -37, -37) },
        { S(  21, -12), S( -17, -59), S(  42, -34), S( -31, -20), S( -14, -14), S(   6, -27), S(  13, -38), S(  -8, -58) },
        { S( -27, -17), S(  36, -10), S(  -1, -10), S(  18,  -4), S(  -1,   7), S(  -6,  -9), S(  16, -34), S(  15, -18) },
        { S(  43, -29), S( -15,  -8), S(   2,  18), S(  34,  12), S(  33,  11), S( -15,   5), S(  10,  -9), S(  52, -37) },
        { S( -36,  -2), S(  -3,  23), S(   7,   8), S(  46,  29), S(  33,  15), S(  54,  29), S(   1,  12), S( -16,  17) },
        { S( -20,  17), S(  13,  27), S( -48,  38), S(  13,  17), S(  28,  28), S( -38,  48), S(  57,  35), S(  34,  28) },
        { S( -62,  13), S( -31,  32), S( -17,  21), S( -93,  40), S( -98,  34), S( -17,  24), S(-105,  10), S( -35,   6) },
        { S( -77,  22), S( -38,  22), S( -86,  49), S(-105,  32), S( -76,  35), S(-142,  10), S(  27,  11), S( -78,   6) }
    },

    // Rook
    {
        { S( -74, -27), S( -51, -40), S( -47, -32), S( -32, -53), S( -20, -69), S( -38, -50), S( -51, -48), S( -63, -39) },
        { S( -91, -32), S( -67, -43), S( -54, -33), S( -34, -44), S( -35, -50), S( -26, -63), S( -16, -58), S( -68, -32) },
        { S( -69, -13), S( -55,  -1), S( -54,  -4), S( -45, -12), S( -23, -24), S( -33, -10), S(  21, -22), S( -12, -23) },
        { S( -64,  36), S( -48,  47), S( -36,  36), S( -16,  19), S( -22,  23), S( -31,  42), S(   3,  38), S( -29,  18) },
        { S( -17,  71), S(  11,  78), S(  19,  57), S(  48,  43), S(  60,  36), S(  48,  65), S(  77,  72), S(  38,  55) },
        { S(  17,  86), S(  63,  74), S(  49,  77), S(  80,  53), S( 101,  54), S(  88,  71), S(  99,  71), S(  57,  84) },
        { S(  13,  76), S(   3,  84), S(  30,  68), S(  40,  59), S(  43,  63), S(  44,  61), S(  30,  86), S(  59,  69) },
        { S(  19,  59), S(  39,  62), S( -42,  72), S( -58,  64), S( -36,  65), S( -27,  74), S(  49,  63), S(  61,  65) }
    },

    // Queen
    {
        { S(  16,-104), S(   2,-114), S(  12,-121), S(   4,-100), S(  13,-134), S( -33,-174), S( -36,-149), S(  18, -97) },
        { S(  -4, -86), S(  10,-103), S(  10,-115), S(  24,-100), S(  18, -97), S(  16,-143), S(   1,-151), S(  29, -92) },
        { S(  -8, -54), S( -13, -55), S(   1, -28), S(  -6, -35), S(  10, -32), S(  17, -37), S(  33, -38), S(  45, -46) },
        { S( -23, -12), S( -25,   2), S( -15,  19), S( -26,  75), S( -13,  43), S(   8,  36), S(  39,  14), S(  42,  28) },
        { S(  -7,  13), S( -19,  53), S( -14,  79), S( -17, 103), S(  -3, 109), S(  20,  78), S(  55,  91), S(  58,  57) },
        { S(  -1,  14), S(   8,  50), S( -17,  93), S(   5,  99), S(  17, 112), S(  67, 120), S(  85,  73), S(  73,  73) },
        { S( -17,  26), S( -24,  32), S( -13,  82), S( -28, 114), S( -13, 121), S(  29,  88), S(  51,  58), S(  80,  80) },
        { S(  21,  -1), S(  35,  23), S(  22,  32), S(   6,  52), S(  41,  57), S(  68,  65), S(  85,  46), S(  87,  42) }
    },

    // King
    {
        { S( 259, -34), S( 305,  43), S( 240,  44), S( 119,  63), S( 135,  44), S( 153,  40), S( 279,   6), S( 254, -60) },
        { S( 335,  66), S( 290,  84), S( 214, 106), S( 186, 104), S( 202,  99), S( 229,  86), S( 303,  42), S( 278,  14) },
        { S( 207,  98), S( 264, 120), S( 203, 141), S( 187, 152), S( 187, 148), S( 192, 126), S( 245,  85), S( 171,  70) },
        { S( 181, 145), S( 230, 177), S( 205, 188), S( 174, 200), S( 183, 192), S( 199, 168), S( 208, 148), S( 142, 124) },
        { S( 181, 171), S( 229, 224), S( 170, 243), S( 146, 244), S( 135, 237), S( 168, 237), S( 213, 204), S( 170, 161) },
        { S( 176, 186), S( 196, 260), S( 150, 268), S( 121, 266), S( 114, 253), S( 162, 267), S( 216, 258), S( 179, 172) },
        { S( 170, 135), S( 189, 249), S( 158, 246), S( 123, 230), S( 128, 237), S( 157, 245), S( 201, 255), S( 168, 137) },
        { S( 111,  41), S( 164, 147), S( 115, 153), S(  73, 156), S(  86, 167), S( 130, 161), S( 161, 151), S( 107,  31) }
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

            psqEntry = pieceValue + PieceBonus[piece][sq_rank(square)][sq_file(square)];

            PsqScore[piece][square] = psqEntry;
            PsqScore[opposite_piece(piece)][opposite_sq(square)] = -psqEntry;
        }
    }
}
