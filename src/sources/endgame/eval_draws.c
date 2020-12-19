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

score_t eval_draw(const board_t *board, color_t winning)
{
    (void)winning;
    score_t score = endgame_score(board->psq_scorepair);

    return ((board->side_to_move == WHITE ? score : -score) / 128);
}

score_t eval_likely_draw(const board_t *board, color_t winning)
{
    (void)winning;
    score_t score = endgame_score(board->psq_scorepair);

    return ((board->side_to_move == WHITE ? score : -score) / 32);
}

score_t eval_tricky_draw(const board_t *board, color_t winning)
{
    (void)winning;
    score_t score = endgame_score(board->psq_scorepair);

    return ((board->side_to_move == WHITE ? score : -score) / 8);
}
