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
    { S( -8, 24), S(-17, 10), S(-23,  7), S(  8,-32), S(  4, 12), S( 32, 10), S( 34,  0), S(  8, -9) },
    { S( -7, 22), S( -7, 14), S( -6, 11), S(  0,  7), S(  9, 17), S( 12,  7), S( 34, -7), S( 18,-12) },
    { S( -5, 29), S(-15, 16), S(  3, -5), S(  5,-16), S(  1, -6), S( 24,  4), S(  5, -1), S(  2,  1) },
    { S( -4, 53), S( -9, 29), S(  2,  9), S( 18,-34), S( 32,-18), S( 49,-14), S(  7,  6), S(  8, 18) },
    { S( 17, 82), S( 17, 79), S( 15, 24), S( 17, -3), S( 32,-10), S(108, 14), S( 51, 29), S( 17, 32) },
    { S(129, 56), S(103, 53), S( 98, 24), S( 96,-57), S(119,-70), S( 81, -2), S(-88, 56), S(-52, 41) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -66, -18), S( -10, -14), S( -11, -24), S(  -8,  16) },
        { S( -14,  -2), S( -32,  12), S( -15,   8), S(  -4,   2) },
        { S( -23, -39), S(  -4,   8), S(  -7,   6), S(   8,  31) },
        { S(  -1,  12), S(  19,  29), S(  10,  58), S(  -5,  51) },
        { S(  36,  25), S(   5,  40), S(  29,  47), S(  13,  62) },
        { S( -32,  14), S(  10,  29), S(  17,  67), S(  16,  49) },
        { S(   2,   6), S(   2,  21), S(  52,   3), S(  44,  32) },
        { S(-178, -54), S(-122,   6), S(-108,  47), S(   0,  41) }
    },

    // Bishop
    {
        { S( -13,  -1), S(  -7,  -7), S( -11,  17), S( -22,  24) },
        { S(  11,  -9), S(   6,   1), S(  15,  -6), S( -13,  25) },
        { S(   1, -10), S(  11,  16), S(   0,  35), S(   1,  36) },
        { S(  10,  10), S(  12,  31), S(  -8,  41), S(  14,  45) },
        { S( -19,  29), S(  -3,  36), S(   8,  41), S(  33,  40) },
        { S(   1,  23), S(   5,  44), S(  26,  43), S(  32,   6) },
        { S( -49,  26), S( -29,  64), S( -33,  33), S( -46,  40) },
        { S( -49,  50), S( -34,  41), S(-160,  82), S(-115,  70) }
    },

    // Rook
    {
        { S( -26,   1), S( -23,   5), S( -20,   9), S( -19,   0) },
        { S( -54,   3), S( -33,   0), S( -30,   3), S( -35,   4) },
        { S( -52,  16), S( -24,  13), S( -36,  28), S( -28,  13) },
        { S( -44,  39), S( -47,  56), S( -47,  52), S( -28,  39) },
        { S( -15,  54), S(   1,  61), S(  -6,  50), S(  19,  53) },
        { S( -15,  73), S(  21,  64), S(  25,  68), S(  33,  61) },
        { S(  -8,  72), S( -26,  87), S(  15,  70), S(  38,  71) },
        { S(  17,  63), S(  27,  63), S(  -2,  76), S(  -8,  77) }
    },

    // Queen
    {
        { S(  12, -70), S(  26, -79), S(   7, -89), S(  19, -56) },
        { S(  20, -43), S(  30, -92), S(  20, -53), S(  21, -30) },
        { S(  10, -37), S(  11, -20), S(  21,  26), S(   3,  15) },
        { S(  20,   7), S(  20,  27), S(   2,  50), S(   6,  74) },
        { S(  14,  43), S(  19,  82), S(   4,  88), S(   0, 119) },
        { S(  20,  59), S(  16,  64), S(   7, 135), S(  14, 127) },
        { S(  -8,  59), S( -34,  84), S(   9, 122), S( -45, 179) },
        { S(  -6,  75), S( -14,  93), S(  -4, 131), S( -15, 125) }
    },

    // King
    {
        { S( 241, -14), S( 275,  36), S( 190,  76), S( 171,  56) },
        { S( 241,  56), S( 257,  84), S( 214, 118), S( 188, 126) },
        { S( 212,  69), S( 244,  90), S( 225, 133), S( 216, 151) },
        { S( 175,  96), S( 250, 126), S( 250, 158), S( 218, 175) },
        { S( 210, 126), S( 243, 165), S( 261, 184), S( 227, 194) },
        { S( 232, 119), S( 293, 178), S( 309, 194), S( 303, 192) },
        { S( 199,  61), S( 246, 154), S( 312, 181), S( 306, 173) },
        { S( 232,-172), S( 373,  84), S( 322,  99), S( 293, 130) }
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
