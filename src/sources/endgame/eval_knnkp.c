/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2020 Morgan Houppin
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

#include "endgame.h"

score_t eval_knnkp(const board_t *board, color_t winning)
{
    square_t    losing_king = board_king_square(board, not_color(winning));
    square_t    losing_pawn = first_square(piecetype_bb(board, PAWN));
    score_t     score = PAWN_EG_SCORE + edge_bonus(losing_king)
        - 4 * relative_square_rank(losing_pawn, not_color(winning));

    return (board->side_to_move == winning ? score : -score);
}