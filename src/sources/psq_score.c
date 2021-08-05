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
    { S( -6, 28), S( -7, 23), S(-15, 10), S( -2, -8), S( -3, 17), S( 30, 23), S( 38,  6), S( 11,-14) },
    { S( -3, 20), S(-21, 17), S( -8,  2), S( -1,  3), S(  2, 11), S(  2, 21), S( 25,-12), S( 14,-12) },
    { S( -8, 19), S(-12, 20), S(  4,-12), S(  6,-25), S( 10,-17), S( 17, -6), S( 13, -1), S( -2, -3) },
    { S(  3, 41), S(  4, 21), S(  2,  6), S( 22,-30), S( 25,-16), S( 41,-23), S( 26,  5), S( 18, 11) },
    { S(  4, 84), S(  9, 67), S( 20, 27), S( 23,-24), S( 36,-30), S(100,-26), S( 70, 14), S( 41, 31) },
    { S( 94, 47), S(100, 27), S(107,  4), S( 73,-67), S(147,-72), S( 37,-62), S(-65, 39), S(-56, 82) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -62, -38), S( -13, -19), S(   4, -14), S( -25,  12) },
        { S( -18,  -3), S( -18,   3), S( -14,  -4), S(  -8,   9) },
        { S( -20, -18), S( -12,  17), S( -10,  12), S(   3,  42) },
        { S(  -6,  22), S(  21,  31), S(   3,  62), S(  10,  61) },
        { S(  15,  19), S(  11,  37), S(  25,  54), S(  12,  68) },
        { S( -12,  33), S(  29,  20), S(  24,  70), S(  28,  46) },
        { S( -32,  10), S( -47,  29), S(  30,   6), S(  60,  21) },
        { S(-183, -23), S( -75,  57), S(-119,  48), S(  -5,  21) }
    },

    // Bishop
    {
        { S(   5,   5), S(  11,  16), S( -11,  18), S( -22,  26) },
        { S(   3,  12), S(   8,   5), S(   4,   2), S( -15,  27) },
        { S(  -1,  16), S(   5,  28), S(  -5,  35), S(  -4,  40) },
        { S(   3,   1), S(  -8,  31), S(  -7,  47), S(  23,  48) },
        { S( -25,  23), S(  -3,  37), S(  18,  33), S(  19,  57) },
        { S(   2,  12), S(   9,  55), S(  38,  33), S(  30,  23) },
        { S( -22,  30), S( -13,  38), S( -26,  31), S(  -8,  40) },
        { S( -42,  52), S( -88,  44), S(-164,  60), S(-122,  56) }
    },

    // Rook
    {
        { S( -33,   7), S( -29,  17), S( -22,  12), S( -20,  15) },
        { S( -64,  20), S( -44,   7), S( -24,  11), S( -12, -10) },
        { S( -41,  20), S( -28,  35), S( -45,  19), S( -38,  27) },
        { S( -46,  43), S( -20,  58), S( -36,  57), S( -41,  51) },
        { S( -30,  65), S(  13,  60), S(   3,  64), S(  29,  43) },
        { S(   6,  68), S(  32,  54), S(  11,  72), S(  59,  44) },
        { S(  10,  69), S( -14,  82), S(  31,  63), S(  34,  68) },
        { S(   6,  70), S(  13,  63), S(   2,  70), S(  17,  67) }
    },

    // Queen
    {
        { S(  27, -83), S(  21, -60), S(  12, -82), S(  24, -49) },
        { S(   8, -56), S(  19, -56), S(  21, -45), S(  17, -32) },
        { S(  19, -27), S(  14, -15), S(  15,  15), S(  -1,  29) },
        { S(  17, -12), S(  13,  36), S(   7,  58), S(  -3,  83) },
        { S(  28,  51), S(   6,  86), S(   4, 100), S(  -9, 121) },
        { S(  15,  53), S(  26,  57), S(  -7, 124), S(  -9, 140) },
        { S(   7,  60), S( -39, 100), S(  -5, 122), S( -51, 173) },
        { S( -26,  76), S( -31, 106), S( -21, 108), S( -23, 120) }
    },

    // King
    {
        { S( 245, -19), S( 275,  35), S( 200,  65), S( 169,  67) },
        { S( 254,  49), S( 239,  89), S( 210, 117), S( 184, 120) },
        { S( 182,  85), S( 224, 101), S( 205, 138), S( 221, 154) },
        { S( 151, 101), S( 262, 137), S( 240, 158), S( 224, 175) },
        { S( 203, 124), S( 292, 158), S( 256, 181), S( 213, 195) },
        { S( 272, 152), S( 305, 198), S( 292, 203), S( 265, 175) },
        { S( 252,  86), S( 293, 189), S( 330, 171), S( 345, 166) },
        { S( 318,-170), S( 410,  48), S( 382,  92), S( 324, 122) }
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
