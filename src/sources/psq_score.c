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
    { S( -2, 32), S( -5, 30), S(-18, 15), S(-12,-10), S(-16, 29), S( 32, 30), S( 34, 11), S(  8,-23) },
    { S(  3, 22), S(-20, 14), S( -8,  4), S( -2,  2), S(  5, 11), S( -3, 18), S( 19, -7), S( 11,-15) },
    { S( -2, 25), S( -9, 21), S(  7,-15), S( -1,-29), S(  4,-19), S( 13,-10), S(  5, -9), S( -5, -2) },
    { S( 11, 52), S(  7, 25), S(  2, 10), S( 22,-33), S( 36,-24), S( 50,-32), S( 20, -1), S( 16, 10) },
    { S( 17,102), S(  0, 83), S( 20, 54), S( 38,-17), S( 30,-15), S(135,  4), S( 41, 29), S( 52, 47) },
    { S(112, 61), S(114, 51), S(120, 31), S(110,-48), S(133,-58), S( 51,-42), S(-78, 52), S(-68, 80) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -77, -58), S( -44, -50), S( -34, -20), S( -42,   5) },
        { S( -30, -30), S( -67, -28), S( -39, -19), S( -28,   0) },
        { S( -55, -46), S( -37,  14), S( -29,  20), S(  -5,  43) },
        { S( -26,  12), S(  10,  44), S(  -6,  52), S( -15,  68) },
        { S(  14,  19), S(   4,  31), S(  21,  50), S(  -1,  67) },
        { S( -16,   5), S( -18,  27), S( -30,  91), S( -35,  56) },
        { S( -37, -22), S( -34,   8), S(  10,   6), S( -23,  42) },
        { S(-176, -32), S( -97,  33), S(-159,  30), S( -27,  14) }
    },

    // Bishop
    {
        { S(   8, -22), S(   7, -20), S( -20,   2), S( -53,  14) },
        { S(   3, -14), S(  -4,   2), S(  -3,  -4), S( -29,  21) },
        { S( -20,  -1), S(   7,  12), S(  -9,  26), S(  -7,  31) },
        { S( -12,   3), S( -26,  30), S( -10,  40), S(  14,  46) },
        { S( -30,  37), S(  11,  40), S(  -8,  43), S(  30,  45) },
        { S( -17,  41), S(  13,  59), S( -24,  52), S(  -5,  28) },
        { S( -62,  42), S( -97,  68), S( -55,  40), S( -66,  53) },
        { S(-124,  37), S( -84,  84), S(-239,  60), S(-185,  79) }
    },

    // Rook
    {
        { S( -48,  -5), S( -40,   7), S( -24,   7), S( -18,   1) },
        { S(-100, -12), S( -58, -14), S( -43,  -1), S( -31, -22) },
        { S( -45,   8), S( -32,  15), S( -61,  28), S( -61,  16) },
        { S( -57,  33), S( -41,  47), S( -33,  49), S( -47,  41) },
        { S( -25,  58), S(   9,  61), S(  15,  60), S(  44,  51) },
        { S(  -3,  74), S(  48,  43), S(  33,  73), S(  60,  49) },
        { S(   8,  81), S( -30,  96), S(  27,  77), S(  38,  84) },
        { S( -45, 117), S(   3,  99), S( -68, 117), S( -29, 104) }
    },

    // Queen
    {
        { S(  -3, -69), S(  15, -82), S(  11, -95), S(  21, -71) },
        { S(   6, -71), S(  16, -94), S(  19, -82), S(  13, -57) },
        { S(  14, -53), S(  19, -23), S(  10,   8), S(   4,   4) },
        { S(   4,  31), S(  10,  49), S(  -1,  65), S( -12, 102) },
        { S(  23,  50), S(   8, 116), S(   5, 116), S( -13, 144) },
        { S(  16,  52), S(  38,  72), S(  21, 129), S( -15, 132) },
        { S(   7,  49), S( -47, 103), S(   7, 108), S( -36, 186) },
        { S( -11,  61), S( -31, 112), S( -19, 144), S( -35, 142) }
    },

    // King
    {
        { S( 253, -69), S( 264,  48), S( 182,  61), S( 149,  51) },
        { S( 262,  37), S( 244,  88), S( 201, 128), S( 173, 137) },
        { S( 177,  67), S( 257,  97), S( 239, 144), S( 266, 167) },
        { S( 144,  80), S( 307, 129), S( 280, 175), S( 242, 200) },
        { S( 186, 107), S( 333, 170), S( 270, 197), S( 241, 210) },
        { S( 260, 118), S( 343, 191), S( 327, 210), S( 289, 186) },
        { S( 228,  66), S( 304, 178), S( 334, 167), S( 324, 146) },
        { S( 269,-165), S( 388,   9), S( 375,  74), S( 314, 112) }
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
