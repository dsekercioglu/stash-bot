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
    { S(-12, 16), S( -6, 17), S(-20, 21), S( -5,  6), S( -2, 13), S( 25, 17), S( 26, 13), S( -7, -7) },
    { S( -7,  4), S(-19, 17), S( -8,  7), S( -5, -0), S(  5, 10), S( -8, 19), S( 12, -5), S( -3, -7) },
    { S( -9, 23), S( -8, 13), S(  0,-12), S(  9,-25), S( 10,-16), S( 13,  0), S(  8, -3), S( -2, -5) },
    { S(  0, 35), S(  1, 18), S(  5, -4), S( 27,-31), S( 37,-24), S( 47,-15), S( 24,  6), S(  2, 19) },
    { S( 21, 66), S( 38, 41), S( 54, 14), S( 57,-26), S( 65,-20), S( 97, 14), S( 62, 38), S( 16, 54) },
    { S( 74, 11), S( 71, 19), S( 88, -7), S(101,-42), S(125,-52), S( 84,-18), S(-40, 54), S(-19, 41) },
    { }
};

const scorepair_t PieceBonus[PIECETYPE_NB][RANK_NB][FILE_NB / 2] = {
    { },
    { },
    
    // Knight
    {
        { S( -53, -52), S( -23, -24), S( -27,  -8), S( -17,  12) },
        { S( -23,   6), S( -17,  12), S(  -7,  -8), S(  -3,   9) },
        { S( -22, -18), S(  -4,  10), S(  -0,  11), S(  14,  45) },
        { S(  -3,  15), S(  26,  34), S(  13,  47), S(  13,  59) },
        { S(   3,  22), S(   7,  34), S(  25,  53), S(  18,  67) },
        { S( -23,  13), S(  16,  21), S(  26,  61), S(  29,  61) },
        { S( -20, -13), S( -30,  12), S(  33,   9), S(  56,  23) },
        { S(-184, -56), S(-132,  22), S(-126,  49), S( -10,   9) }
    },

    // Bishop
    {
        { S(  13,  -9), S(  -5,  14), S( -17,  10), S( -13,  20) },
        { S(   8,  -6), S(  12,  -3), S(   3,   5), S(  -8,  19) },
        { S(   2,  11), S(  13,  20), S(   6,  32), S(  -2,  45) },
        { S(  -5,  12), S(  12,  29), S(  -3,  53), S(  21,  61) },
        { S(  -5,  29), S(  -2,  47), S(  18,  43), S(  27,  59) },
        { S(  -2,  39), S(  18,  59), S(  21,  45), S(  28,  34) },
        { S( -63,  35), S( -43,  48), S( -18,  45), S( -25,  37) },
        { S( -78,  55), S( -69,  44), S(-164,  59), S(-135,  60) }
    },

    // Rook
    {
        { S( -29,   3), S( -26,  10), S( -21,  17), S( -18,   7) },
        { S( -75,  17), S( -33,   7), S( -26,   5), S( -31,   8) },
        { S( -51,  17), S( -27,  22), S( -45,  27), S( -38,  18) },
        { S( -47,  45), S( -27,  46), S( -35,  52), S( -33,  43) },
        { S( -15,  58), S(   5,  61), S(  12,  54), S(  26,  42) },
        { S( -10,  69), S(  40,  46), S(  30,  62), S(  58,  46) },
        { S(  -5,  85), S( -11,  83), S(  25,  76), S(  23,  77) },
        { S(  22,  73), S(  14,  76), S(   3,  85), S(   6,  74) }
    },

    // Queen
    {
        { S(  37, -74), S(  21, -67), S(  20, -71), S(  27, -41) },
        { S(  18, -50), S(  29, -52), S(  29, -53), S(  27, -25) },
        { S(  26, -24), S(  21,  -3), S(  19,  27), S(   5,  19) },
        { S(  16,  23), S(  16,  41), S(  -2,  65), S(   2,  93) },
        { S(  19,  50), S(   5,  90), S(   4,  91), S(  -2, 115) },
        { S(   8,  65), S(  15,  64), S(   3, 111), S(  -1, 116) },
        { S(  -8,  59), S( -62, 107), S( -12, 118), S( -48, 157) },
        { S( -21,  65), S( -29,  95), S( -20, 117), S( -22, 122) }
    },

    // King
    {
        { S( 253,  -7), S( 283,  56), S( 209,  88), S( 206,  72) },
        { S( 256,  61), S( 261,  98), S( 229, 121), S( 199, 127) },
        { S( 167,  85), S( 239, 106), S( 250, 124), S( 242, 137) },
        { S( 157,  84), S( 271, 117), S( 267, 137), S( 242, 152) },
        { S( 192, 113), S( 287, 147), S( 278, 162), S( 241, 168) },
        { S( 226, 128), S( 333, 170), S( 323, 163), S( 291, 153) },
        { S( 208,  81), S( 268, 174), S( 303, 154), S( 286, 139) },
        { S( 274,-160), S( 346,  37), S( 316,  62), S( 264,  91) }
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
