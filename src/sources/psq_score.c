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

score_t PsqScore[PIECE_NB][SQUARE_NB] = {
    { },

    // Pawn
    {
          0,   0,   0,   0,   0,   0,   0,   0,
        106, 118,  94,  94,  98, 125, 136, 104,
        109, 122, 107, 108, 115, 111, 134, 110,
        113, 128, 114, 118, 120, 115, 129, 105,
        134, 143, 128, 129, 131, 129, 141, 123,
        182, 193, 186, 176, 174, 175, 189, 171,
        255, 249, 230, 205, 210, 196, 188, 215,
          0,   0,   0,   0,   0,   0,   0,   0
    },

    // Knight
    {
        297, 330, 318, 317, 325, 323, 331, 280,
        313, 327, 345, 351, 348, 351, 338, 327,
        334, 355, 357, 374, 374, 362, 358, 333,
        356, 367, 385, 379, 381, 377, 377, 354,
        366, 379, 399, 405, 391, 404, 375, 382,
        356, 392, 400, 408, 407, 407, 401, 356,
        347, 362, 390, 391, 375, 390, 347, 347,
        243, 353, 358, 357, 356, 341, 330, 246
    },

    // Bishop
    {
        370, 371, 364, 359, 363, 358, 355, 367,
        381, 389, 388, 379, 384, 390, 399, 385,
        384, 393, 398, 396, 396, 396, 390, 385,
        375, 401, 400, 413, 406, 394, 392, 379,
        387, 398, 411, 415, 412, 411, 394, 392,
        388, 407, 404, 416, 407, 419, 408, 410,
        381, 397, 397, 389, 397, 395, 389, 360,
        384, 383, 383, 390, 381, 380, 380, 403
    },

    // Rook
    {
        575, 579, 588, 590, 588, 576, 566, 555,
        559, 571, 580, 583, 575, 575, 573, 551,
        565, 582, 585, 583, 578, 572, 579, 566,
        589, 602, 603, 601, 594, 587, 591, 582,
        614, 621, 624, 624, 616, 619, 608, 607,
        629, 629, 632, 632, 631, 637, 629, 622,
        635, 633, 644, 641, 637, 640, 628, 628,
        623, 622, 626, 622, 622, 621, 617, 620
    },

    // Queen
    {
        1120, 1101, 1105, 1118, 1102, 1083, 1058, 1089,
        1106, 1117, 1123, 1117, 1119, 1115, 1109, 1101,
        1113, 1123, 1128, 1120, 1121, 1128, 1128, 1122,
        1118, 1133, 1132, 1144, 1142, 1142, 1140, 1137,
        1124, 1134, 1150, 1158, 1176, 1182, 1170, 1160,
        1121, 1140, 1153, 1169, 1180, 1214, 1208, 1188,
        1124, 1124, 1167, 1177, 1185, 1207, 1186, 1203,
        1135, 1155, 1171, 1166, 1181, 1191, 1187, 1199,
    },

    // King
    {
        -29,  -7, -21, -54, -33, -45, -12, -27,
        -21, -17, -16, -15, -18, -13, -14, -21,
        -25,  -9,  -6,  -1,  -1,  -5,  -9, -26,
        -29,   3,  10,  11,  13,   9,   6, -20,
         -1,  14,  13,  21,  21,  25,  29,   1,
        -11,  13,   6,  14,  21,  31,  34,   9,
          1,  27,  -1,   1,   9,  26,  43,   5,
       -124,  54,  30,  -2,   5,  43,  61, -68,
    }
};

void psq_score_init(void) {
    for (piece_t pc = BLACK_PAWN; pc <= BLACK_KING; ++pc)
        for (square_t sq = SQ_A1; sq <= SQ_H8; ++sq)
            PsqScore[pc][sq] = -PsqScore[opposite_piece(pc)][opposite_sq(sq)];
}