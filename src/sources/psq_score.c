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
    { S(-12, 20), S( -5,  8), S(-20, 20), S(-18, -4), S( -3, 35), S( 35, 21), S( 36,  7), S(  7,-24) },
    { S( -1,  8), S(-12, 23), S(  2, 12), S(  3,  5), S(  9,  7), S( 20, 13), S( 30, -2), S( 14,-10) },
    { S( -8, 20), S(-13, 21), S( -1, -3), S(  5,-26), S(  2,-13), S( 12,  1), S(  3, -5), S(  3,  1) },
    { S(-17, 52), S( -2, 32), S( -4,  9), S( 26,-18), S( 41,-21), S( 51,-21), S( 13, 12), S( 14,  6) },
    { S(  8,114), S(  8, 78), S( 23, 55), S( 42, 12), S( 39,-17), S(128,  8), S( 47, 50), S( 26, 54) },
    { S(120, 49), S( 99, 42), S( 83, 13), S(116,-29), S(133,-64), S( 89,-25), S(-54, 38), S(-24, 33) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -71, -74), S( -32, -47), S( -28, -23), S( -28,  -5) },
        { S( -33, -26), S( -34,  -9), S( -22, -27), S( -10,  14) },
        { S( -31, -49), S(  -6,  -3), S( -12,   8), S(   2,  48) },
        { S(  -8,  11), S(  30,  23), S(  16,  55), S(  -5,  67) },
        { S(  27,  15), S(   9,  24), S(  29,  53), S(  14,  75) },
        { S(   5,   8), S(   8,  31), S(  -7,  72), S(  -8,  52) },
        { S(  -8, -34), S( -22,  -7), S(  36,  -3), S(  27,  29) },
        { S(-184, -40), S(-122,  27), S(-137,  42), S( -20,   0) }
    },

    // Bishop
    {
        { S(   6,  -3), S(   4,   3), S( -16,  -4), S( -42,   4) },
        { S( -13, -30), S(   9,   3), S(   2,   1), S( -17,  12) },
        { S(   3,  29), S(   2,  20), S(  -8,  35), S(   4,  35) },
        { S( -13,   8), S(  -4,  31), S(  -5,  43), S(   8,  42) },
        { S( -17,  27), S(  25,  47), S(  15,  30), S(  36,  53) },
        { S(  -2,  50), S(  -5,  39), S(  19,  48), S(  17,  38) },
        { S( -36,  24), S( -47,  42), S( -26,  33), S( -56,  28) },
        { S( -92,  60), S( -64,  49), S(-179,  56), S(-148,  64) }
    },

    // Rook
    {
        { S( -36,   1), S( -25,  10), S( -17,   7), S( -18,  -2) },
        { S( -89, -13), S( -30, -15), S( -33,  -1), S( -44, -14) },
        { S( -54,  -4), S( -33,  18), S( -56,  14), S( -40,  12) },
        { S( -51,  37), S( -28,  53), S( -43,  50), S( -33,  50) },
        { S( -14,  61), S(  -1,  62), S(  12,  54), S(  42,  58) },
        { S( -11,  79), S(  31,  46), S(  35,  67), S(  55,  49) },
        { S(   0,  77), S( -11,  88), S(  26,  77), S(  34,  88) },
        { S(   5,  83), S(  22,  78), S( -31, 103), S(  -0, 102) }
    },

    // Queen
    {
        { S(  25, -68), S(  25,-104), S(  10, -74), S(  27, -43) },
        { S(  14, -81), S(  16, -91), S(  30, -67), S(  16, -44) },
        { S(  20, -41), S(  26, -10), S(  14,  11), S(  10,  10) },
        { S(   8,   6), S(  20,  37), S(   4,  57), S(  -1,  96) },
        { S(  14,  35), S(  23,  88), S(   4,  99), S( -15, 117) },
        { S( -17,  61), S(  24,  58), S(  17, 124), S(  -1, 134) },
        { S(  -6,  55), S( -52,  88), S( -15, 106), S( -49, 187) },
        { S( -26,  57), S( -17, 103), S( -21, 128), S( -23, 134) }
    },

    // King
    {
        { S( 247, -45), S( 262,  48), S( 195,  68), S( 149,  53) },
        { S( 260,  56), S( 245,  95), S( 195, 123), S( 178, 127) },
        { S( 194,  78), S( 259, 102), S( 245, 138), S( 239, 156) },
        { S( 156,  89), S( 303, 127), S( 290, 164), S( 275, 180) },
        { S( 192, 108), S( 311, 160), S( 306, 185), S( 265, 190) },
        { S( 235, 125), S( 358, 178), S( 350, 184), S( 302, 175) },
        { S( 209,  92), S( 278, 187), S( 300, 153), S( 278, 151) },
        { S( 233,-146), S( 324,  21), S( 278,  54), S( 246,  92) }
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
