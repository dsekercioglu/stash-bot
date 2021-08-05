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
    { S( -3, 36), S( -4, 27), S(-22,  9), S(-11, -7), S(-12, 21), S( 28, 27), S( 40,  6), S( 12,-14) },
    { S(  4, 21), S(-22, 21), S( -8,  8), S( -3, 11), S(  0, 14), S( -6, 27), S( 25, -8), S( 19,-17) },
    { S( -2, 23), S(-13, 23), S( 10,-14), S(  8,-27), S( 10,-16), S( 17,-10), S(  4, -0), S( -6, -1) },
    { S( 12, 51), S(  9, 33), S(  6, 15), S( 24,-30), S( 25,-19), S( 46,-36), S( 25,  5), S( 24, 12) },
    { S( 20,101), S(  4, 77), S( 25, 45), S( 30,-13), S( 22,-18), S(109,-12), S( 71,  4), S( 47, 39) },
    { S( 93, 71), S(108, 41), S(117, 19), S( 90,-61), S(115,-71), S( 52,-44), S(-63, 53), S(-31, 80) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -59, -53), S( -32, -43), S( -22, -40), S( -38,   1) },
        { S( -24, -20), S( -47,  -3), S( -30, -20), S( -19,  -1) },
        { S( -35, -27), S( -19,   6), S( -21,  12), S(   2,  39) },
        { S( -12,   8), S(   6,  32), S(  -1,  60), S(  -2,  68) },
        { S(  17,  12), S(  10,  36), S(  21,  53), S(   2,  63) },
        { S(  -2,  38), S(  12,  16), S( -26,  86), S( -11,  63) },
        { S( -29,  -9), S( -26,  32), S(  21,  27), S( -14,  38) },
        { S(-173, -46), S(-105,  65), S(-166,  48), S(  -2,  15) }
    },

    // Bishop
    {
        { S(   7, -15), S(  20,  -8), S( -19,   6), S( -46,  21) },
        { S(   9, -12), S(  10,   4), S(   4, -14), S( -20,  14) },
        { S(  -5,   4), S(   6,  23), S(  -5,  23), S(  -5,  34) },
        { S( -11,   6), S( -23,  28), S(  -9,  45), S(  14,  58) },
        { S( -31,  10), S( -13,  47), S(   3,  40), S(  18,  50) },
        { S(  -6,  22), S(  19,  59), S( -34,  37), S( -10,  41) },
        { S( -43,  25), S( -89,  44), S( -49,  49), S( -77,  45) },
        { S( -81,  49), S( -92,  50), S(-225,  62), S(-149,  50) }
    },

    // Rook
    {
        { S( -39,  -9), S( -31,  -2), S( -23,  -1), S( -20,   4) },
        { S( -81,   2), S( -58, -23), S( -39,   1), S( -20, -22) },
        { S( -33,  20), S( -39,  36), S( -58,  27), S( -53,  31) },
        { S( -35,  37), S( -31,  55), S( -41,  56), S( -46,  56) },
        { S( -23,  73), S(  22,  76), S(   6,  61), S(  36,  56) },
        { S(   5,  79), S(  45,  58), S(  26,  74), S(  55,  59) },
        { S(  11,  73), S(   2,  89), S(  42,  68), S(  45,  81) },
        { S( -22,  90), S(  -3,  71), S( -62,  93), S( -64,  94) }
    },

    // Queen
    {
        { S(  12, -84), S(   8,-107), S(   7,-110), S(  18, -83) },
        { S(  -0, -77), S(  15, -76), S(  16, -60), S(  13, -62) },
        { S(  20, -32), S(  13, -18), S(  21,   7), S(  -2,  15) },
        { S(   7,  24), S(  11,  36), S(   7,  61), S( -14, 114) },
        { S(  37,  52), S(  12, 115), S(  18, 107), S(   7, 153) },
        { S(  34,  72), S(  23,  73), S(  10, 143), S(  12, 149) },
        { S(  18,  64), S( -41, 108), S(  19, 130), S( -42, 192) },
        { S(  -2,  86), S( -28,  92), S( -25, 137), S( -25, 132) }
    },

    // King
    {
        { S( 243, -50), S( 279,  30), S( 179,  52), S( 163,  56) },
        { S( 254,  34), S( 242,  81), S( 204, 123), S( 178, 127) },
        { S( 176,  63), S( 227,  99), S( 209, 144), S( 245, 159) },
        { S( 143,  89), S( 264, 141), S( 272, 157), S( 247, 187) },
        { S( 218, 137), S( 304, 180), S( 269, 207), S( 224, 205) },
        { S( 252, 168), S( 349, 207), S( 304, 229), S( 303, 202) },
        { S( 242, 102), S( 316, 203), S( 352, 190), S( 307, 160) },
        { S( 298,-155), S( 385,  64), S( 328,  51), S( 296, 103) }
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
