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
#include "movelist.h"
#include "tt.h"
#include "uci.h"

void	search_bestmove(board_t *board, int depth, root_move_t *begin,
		root_move_t *end, int pv_line)
{
	extern goparams_t	g_goparams;
	score_t				alpha = -INF_SCORE;
	searchstack_t		sstack[512];
	move_t				pv[512];

	memset(sstack, 0, sizeof(sstack));

	sstack[0].plies = 1;
	sstack[0].pv = pv;

	for (root_move_t *i = begin; i < end; ++i)
	{
		pthread_mutex_lock(&g_engine_mutex);

		if (g_engine_send == DO_ABORT || g_engine_send == DO_EXIT)
		{
			i->score = -NO_SCORE;
			pthread_mutex_unlock(&g_engine_mutex);
			return ;
		}
		pthread_mutex_unlock(&g_engine_mutex);

		boardstack_t	stack;

		do_move(board, i->move, &stack);

		clock_t			elapsed = chess_clock() - g_goparams.start;

		if (elapsed > 3000)
		{
			uint64_t	nps = (g_nodes * 1000) / elapsed;

			printf("info depth %d nodes %lu nps %lu"
				" time %lu currmove %s currmovenumber %d\n",
				depth + 1, (info_t)g_nodes, (info_t)nps, elapsed,
				move_to_str(i->move, board->chess960),
				(int)(i - begin) + pv_line + 1);
			fflush(stdout);
		}

		pv[0] = NO_MOVE;

		if (i == begin)
			i->score = -search(board, depth, -INF_SCORE, INF_SCORE, sstack, true);
		else
		{
			i->score = -search(board, depth, -alpha - 1, -alpha, sstack, false);

			if (alpha < i->score)
			{
				pv[0] = NO_MOVE;
				i->score = -search(board, depth, -INF_SCORE,
					-i->score, sstack, true);
			}
		}

		undo_move(board, i->move);

		if (abs(i->score) > INF_SCORE)
			return ;
		else if (i->score > alpha)
		{
			alpha = i->score;
			i->depth = depth + 1;
			i->pv[0] = i->move;

			size_t	j;
			for (j = 0; sstack[0].pv[j] != NO_MOVE; ++j)
				i->pv[j + 1] = sstack[0].pv[j];

			i->pv[j + 1] = NO_MOVE;
		}
	}
	return ;
}
