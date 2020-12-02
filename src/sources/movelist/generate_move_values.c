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

#include "history.h"
#include "lazy_smp.h"
#include "movelist.h"

void    generate_move_values(movelist_t *movelist, const board_t *board,
        move_t tt_move, move_t *killers)
{
    static const score_t MVV_Table[PIECETYPE_NB] = {
        0, 0, 72, 72, 144, 288, 0, 0
    };
    worker_t *const     worker = get_worker(board);
    extmove_t *const    end = movelist->last;

    for (extmove_t *extmove = movelist->moves; extmove < end; ++extmove)
    {
        move_t  move = extmove->move;

        if (move == tt_move)
        {
            extmove->score = 16384;
            continue ;
        }

        square_t  from = move_from_square(move);
        piece_t   moved_piece = piece_on(board, from);

        if (is_capture_or_promotion(board, move))
        {
            square_t    to = move_to_square(move);
            piecetype_t captured = type_of_piece(piece_on(board, to));

            if (type_of_move(move) == PROMOTION)
            {
                captured = PAWN;
                extmove->score = promotion_type(move) == QUEEN ? 8192 : -8192;
            }
            else if (type_of_move(move) == EN_PASSANT)
            {
                captured = PAWN;
                extmove->score = 4096;
            }
            else
                extmove->score = see_greater_than(board, move, 0) ? 4096 : 1024;

            extmove->score += MVV_Table[captured];
            extmove->score += get_chistory_score(worker->chistory, moved_piece, to, captured);
        }
        else if (move == killers[0] || move == killers[1])
            extmove->score = 2048;
        else
            extmove->score = get_qhistory_score(worker->qhistory, moved_piece, move);
    }
}
