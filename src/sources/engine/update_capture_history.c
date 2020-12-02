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

#include "engine.h"

void    update_single_capture(chistory_t hist, const board_t *board, move_t move,
        int bonus)
{
    square_t    from = move_from_square(move);
    piece_t     moved_piece = piece_on(board, from);
    square_t    to = move_to_square(move);
    piecetype_t captured = type_of_piece(piece_on(board, to));

    if (type_of_move(move) == PROMOTION)
        captured = PAWN;
    else if (type_of_move(move) == EN_PASSANT)
        captured = PAWN;

    add_chistory(hist, moved_piece, to, captured, bonus);
}

void    update_capture_history(chistory_t hist, const board_t *board, int depth,
        move_t bestmove, const move_t captures[64], int ccount)
{
    int bonus = history_bonus(depth);

    update_single_capture(hist, board, bestmove, bonus);

    for (int i = 0; i < ccount; ++i)
        update_single_capture(hist, board, captures[i], -bonus);
}
