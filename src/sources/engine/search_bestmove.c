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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"
#include "history.h"
#include "imath.h"
#include "info.h"
#include "lazy_smp.h"
#include "movelist.h"
#include "timeman.h"
#include "tt.h"

void    search_bestmove(board_t *board, int depth, score_t alpha, score_t beta,
        root_move_t *begin, root_move_t *end, int pv_line)
{
    searchstack_t       sstack[512];
    move_t              pv[512];
    worker_t            *worker = get_worker(board);

    memset(sstack, 0, sizeof(sstack));

    sstack[0].plies = 1;
    sstack[0].pv = pv;

    movelist_t  list;
    int         move_count = 0;

    list_pseudo(&list, board);
    generate_move_values(&list, board, begin->move, sstack[0].killers);

    for (extmove_t *extmove = list.moves; extmove < list.last; ++extmove)
    {
        root_move_t *cur;

        place_top_move(extmove, list.last);
        if ((cur = find_root_move(begin, end, extmove->move)) == NULL)
            continue ;

        move_count++;

        if (!worker->idx && chess_clock() - Timeman.start > 3000)
        {
            printf("info depth %d currmove %s currmovenumber %d\n",
                depth, move_to_str(cur->move, board->chess960),
                move_count + pv_line);
            fflush(stdout);
        }

        boardstack_t    stack;
        score_t         next = -NO_SCORE;
        int             reduction;
        int             new_depth = depth - 1;
        bool            is_quiet = !is_capture_or_promotion(board, cur->move);

        do_move(board, cur->move, &stack);

        // Can we apply LMR ?
        if (depth >= LMR_MinDepth && move_count > LMR_MinMoves && !board->stack->checkers)
        {
            reduction = Reductions[min(depth, 63)][min(move_count, 63)];

            // Adjust LMR based on history
            if (is_quiet)
                reduction -= get_history_score(worker->history,
                    piece_on(board, move_from_square(cur->move)), cur->move) / 220;

            reduction = max(0, reduction);
        }
        else
            reduction = 0;

        if (reduction)
            next = -search(board, new_depth - reduction, -alpha - 1, -alpha, sstack, false);

        // If LMR is not possible, or our LMR failed, do a search with no reductions
        if ((reduction && next > alpha) || (!reduction && move_count != 1))
            next = -search(board, new_depth, -alpha - 1, -alpha, sstack, false);

        // Finally, if null window search and LMR search failed above alpha, do
        // a full window search.
        if (move_count == 1 || next > alpha)
        {
            pv[0] = NO_MOVE;
            next = -search(board, new_depth, -beta, -alpha, sstack, true);
        }

        undo_move(board, cur->move);

        if (g_engine_send == DO_ABORT || g_engine_send == DO_EXIT)
            return ;

        else if (move_count == 1 || next > alpha)
        {
            cur->score = next;
            alpha = max(alpha, next);
            cur->seldepth = worker->seldepth;
            cur->pv[0] = cur->move;

            size_t  j;
            for (j = 0; sstack[0].pv[j] != NO_MOVE; ++j)
                cur->pv[j + 1] = sstack[0].pv[j];

            cur->pv[j + 1] = NO_MOVE;

            if (next >= beta)
                return ;
        }
        else
            cur->score = -INF_SCORE;
    }
    return ;
}
