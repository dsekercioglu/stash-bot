/*
**	Stash, a UCI chess playing engine developed from scratch
**	Copyright (C) 2019-2020 Morgan Houppin
**
**	Stash is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	Stash is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include "tt.h"
#include "uci.h"

void	search_bestmove(board_t *board, int depth, score_t alpha, score_t beta,
		root_move_t *begin, root_move_t *end, int pv_line)
{
	extern goparams_t	g_goparams;
	searchstack_t		sstack[512];
	move_t				pv[512];

	memset(sstack, 0, sizeof(sstack));

	sstack[0].plies = 1;
	sstack[0].pv = pv;

	movelist_t			list;
	int					move_count = 0;

	list_pseudo(&list, board);
	generate_move_values(&list, board, begin->move, NULL);

	for (extmove_t *extmove = list.moves; extmove < list.last; ++extmove)
	{
		root_move_t	*cur;

		place_top_move(extmove, list.last);
		if ((cur = find_root_move(begin, end, extmove->move)) == NULL)
			continue ;

		move_count++;

		clock_t		elapsed = chess_clock() - g_goparams.start;

		if (!get_worker(board)->idx && elapsed > 3000)
		{
			printf("info depth %d currmove %s currmovenumber %d\n",
				depth, move_to_str(cur->move, board->chess960),
				move_count + pv_line);
			fflush(stdout);
		}

		boardstack_t	stack;
		score_t			next;
		int				reduction;
		int				new_depth = depth - 1;
		bool			is_quiet = !is_capture_or_promotion(board, cur->move);

		do_move(board, cur->move, &stack);

		// Can we apply LMR ?
		if (depth >= LMR_MinDepth && move_count > 3 && !board->stack->checkers
			&& (is_quiet || extmove->score < 2048))
		{
			reduction = (depth + move_count) / 10 + 1;
			if (!is_quiet)
				reduction = reduction * 2 / 3;
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
			cur->seldepth = get_worker(board)->seldepth;
			cur->pv[0] = cur->move;

			size_t	j;
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
