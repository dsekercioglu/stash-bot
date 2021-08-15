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
    { S( -5, 25), S( -2, 19), S(-16, 14), S( -1, -6), S(  3, 10), S( 32, 19), S( 43,  7), S( 13,-19) },
    { S( -7, 19), S(-19, 18), S(-14,  8), S( -3,  1), S(  1, 15), S(  5, 16), S( 25, -7), S( 11,-13) },
    { S(-11, 23), S(-10, 16), S( -2,-11), S(  5,-30), S( 10,-16), S( 14, -8), S( 16, -5), S( -3, -2) },
    { S( -1, 50), S(  2, 25), S( -4,  8), S( 21,-33), S( 29,-19), S( 37,-27), S( 23, -1), S( 12, 14) },
    { S( -4, 84), S( 11, 66), S(  7, 29), S( 25,-27), S( 40,-43), S(115,-15), S( 69, 12), S( 43, 26) },
    { S( 81, 59), S(121, 23), S( 97,  1), S(102,-66), S(129,-85), S( 60,-37), S(-92, 43), S(-67, 60) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -38,  -1), S( -12, -18), S( -13, -10), S( -14,   9) },
        { S( -23, -11), S( -30,  16), S( -14,  -5), S(  -6,   6) },
        { S( -21, -27), S( -12,  16), S(  -8,  13), S(   6,  38) },
        { S(  -4,  22), S(  24,  29), S(   6,  51), S(   9,  54) },
        { S(  15,  26), S(  11,  36), S(  31,  45), S(  14,  68) },
        { S( -12,  26), S(  23,  26), S(  27,  63), S(  32,  45) },
        { S( -23,   5), S( -23,  29), S(  35,  20), S(  67,  27) },
        { S(-150,   3), S( -75,  34), S( -92,  63), S(   4,  30) }
    },

    // Bishop
    {
        { S(  -2,   8), S(   7,   6), S(  -9,  19), S( -16,  19) },
        { S(   2, -10), S(   9,  -0), S(   4,   6), S( -13,  20) },
        { S(  -2,  16), S(   3,  24), S(  -4,  32), S(  -2,  41) },
        { S(  -3,   4), S(   0,  27), S(  -5,  47), S(  19,  52) },
        { S(  -8,  22), S(  -5,  44), S(  20,  37), S(  26,  54) },
        { S(  -6,  25), S(  14,  48), S(  30,  34), S(  19,  34) },
        { S( -41,  34), S( -40,  42), S( -30,  47), S( -18,  39) },
        { S( -58,  41), S( -55,  47), S(-110,  53), S(-124,  57) }
    },

    // Rook
    {
        { S( -30,   3), S( -28,  11), S( -19,   8), S( -17,   5) },
        { S( -74,  17), S( -38,   4), S( -28,   5), S( -24,   2) },
        { S( -37,  19), S( -33,  26), S( -44,  31), S( -43,  28) },
        { S( -41,  48), S( -30,  51), S( -27,  47), S( -36,  49) },
        { S( -31,  70), S(  -3,  64), S(   0,  61), S(  21,  48) },
        { S( -15,  77), S(  25,  64), S(  22,  68), S(  45,  47) },
        { S(   3,  74), S( -13,  80), S(  25,  69), S(  47,  61) },
        { S(   2,  77), S(   8,  72), S(   6,  71), S(   9,  70) }
    },

    // Queen
    {
        { S(  21, -63), S(  15, -60), S(  16, -72), S(  25, -57) },
        { S(  12, -53), S(  18, -61), S(  22, -49), S(  18, -28) },
        { S(  23, -36), S(  12,   3), S(  12,  26), S(   5,  17) },
        { S(   4,  25), S(   9,  52), S(   2,  62), S(   1,  81) },
        { S(  21,  39), S(  -4,  97), S(   8,  98), S(  -4, 124) },
        { S(  16,  55), S(  30,  70), S(  -2, 140), S(   1, 128) },
        { S(  -6,  58), S( -43,  97), S(  -4, 122), S( -24, 163) },
        { S( -10,  79), S( -15,  88), S( -20, 104), S( -10, 119) }
    },

    // King
    {
        { S( 246, -17), S( 276,  30), S( 198,  63), S( 166,  73) },
        { S( 261,  37), S( 248,  80), S( 214, 112), S( 173, 129) },
        { S( 190,  75), S( 228, 100), S( 202, 138), S( 203, 159) },
        { S( 147, 103), S( 230, 137), S( 223, 166), S( 194, 183) },
        { S( 202, 122), S( 281, 166), S( 223, 190), S( 163, 208) },
        { S( 284, 143), S( 311, 182), S( 267, 208), S( 256, 190) },
        { S( 286, 101), S( 316, 179), S( 362, 186), S( 366, 158) },
        { S( 336,-163), S( 475,  93), S( 396,  95), S( 365, 127) }
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
