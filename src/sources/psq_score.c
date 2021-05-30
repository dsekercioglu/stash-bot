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
    { S(-15, 21), S(-17, 13), S(-21, 15), S( -4,  1), S(  2, 18), S( 33, 12), S( 29, -1), S(  3, -8) },
    { S(-10, 21), S(-11,  4), S( -5,  9), S( -1, -3), S(  9, 16), S( 13,  7), S( 33, -4), S( 16,-12) },
    { S( -8, 33), S(-22, 18), S(  2, -3), S( 11,-20), S(  1,-10), S( 29,  2), S( -2, -1), S( -3,  3) },
    { S(  1, 57), S(  4, 24), S(  0, 21), S( 26,-29), S( 31,-23), S( 49,-14), S(  6,  5), S( 15,  8) },
    { S( 11, 90), S(  6, 76), S( 26, 46), S( 35, -2), S( 41,-28), S(126, 16), S( 58, 45), S( 15, 38) },
    { S(102, 43), S(106, 55), S( 81, 10), S( 92,-54), S(112,-68), S( 82,-29), S(-73, 43), S(-23, 43) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -53, -54), S( -13, -26), S( -19,   0), S( -10,   9) },
        { S( -20, -14), S( -40, -14), S( -21,  -8), S(   2,  20) },
        { S( -20, -37), S(   2,   8), S(  -9,  15), S(  12,  26) },
        { S( -10,  15), S(  30,  24), S(  14,  48), S(   0,  58) },
        { S(  31,  27), S(  13,  35), S(  28,  49), S(  10,  74) },
        { S(  -4,  12), S(  13,  22), S(  13,  69), S(  18,  62) },
        { S(   0, -21), S(   8,  23), S(  49,  -1), S(  42,  30) },
        { S(-200, -57), S(-122,  40), S(-122,  52), S(   2,  20) }
    },

    // Bishop
    {
        { S(   1,  -2), S(   7,  13), S( -10,  11), S( -25,  31) },
        { S(  11, -14), S(   7,   8), S(  16,   6), S( -10,  18) },
        { S(  -7,   6), S(  15,  29), S(   2,  30), S(   2,  31) },
        { S(   4,   1), S(  -5,  19), S(  -5,  46), S(  15,  45) },
        { S( -13,  31), S(  -1,  32), S(  20,  32), S(  32,  52) },
        { S(  -3,  52), S(  10,  45), S(  17,  41), S(  34,  23) },
        { S( -44,  39), S( -40,  44), S( -22,  38), S( -32,  39) },
        { S( -60,  60), S( -80,  34), S(-151,  61), S(-137,  67) }
    },

    // Rook
    {
        { S( -26,   5), S( -26,   2), S( -22,  24), S( -17,   1) },
        { S( -61,  12), S( -21,  -4), S( -35,  -2), S( -33,   3) },
        { S( -49,  11), S( -31,  14), S( -48,  27), S( -45,  11) },
        { S( -44,  25), S( -22,  53), S( -35,  56), S( -25,  36) },
        { S( -10,  62), S(   4,  64), S(   5,  57), S(  17,  54) },
        { S( -11,  68), S(  45,  59), S(  18,  57), S(  49,  58) },
        { S(  -6,  69), S( -10,  85), S(  14,  65), S(  32,  79) },
        { S(  17,  74), S(  13,  67), S( -11,  83), S(  -8,  79) }
    },

    // Queen
    {
        { S(  17, -69), S(  27, -97), S(  14, -72), S(  23, -54) },
        { S(   9, -65), S(  13, -75), S(  24, -61), S(  27, -41) },
        { S(   8, -58), S(  24,  -1), S(  17,  14), S(  12,   6) },
        { S(  18,   5), S(   9,  32), S(  -5,  50), S(   0,  86) },
        { S(  14,  45), S(  15,  87), S(  13, 101), S(  -7, 127) },
        { S(   3,  73), S(  24,  64), S( -10, 119), S(  11, 131) },
        { S( -12,  48), S( -49, 104), S( -14,  97), S( -48, 178) },
        { S( -14,  66), S(  -4, 113), S( -21, 119), S( -15, 128) }
    },

    // King
    {
        { S( 248, -26), S( 265,  47), S( 185,  76), S( 169,  70) },
        { S( 252,  53), S( 246,  95), S( 214, 127), S( 175, 129) },
        { S( 198,  90), S( 252, 101), S( 222, 141), S( 233, 143) },
        { S( 169,  92), S( 266, 126), S( 272, 148), S( 270, 171) },
        { S( 203, 127), S( 298, 167), S( 295, 184), S( 249, 185) },
        { S( 239, 129), S( 323, 178), S( 323, 176), S( 302, 168) },
        { S( 190,  76), S( 260, 177), S( 287, 144), S( 258, 146) },
        { S( 236,-139), S( 325,  42), S( 274,  48), S( 242,  83) }
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
