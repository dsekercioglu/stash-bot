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
#include "engine.h"
#include "history.h"
#include "imath.h"
#include "info.h"
#include "lazy_smp.h"
#include "pawns.h"
#include "tt.h"
#include "uci.h"

uint64_t	perft(board_t *board, unsigned int depth)
{
	if (depth == 0)
		return (1);
	else
	{
		movelist_t	list;
		list_all(&list, board);
		if (depth == 1)
			return (movelist_size(&list));

		uint64_t		sum = 0;

		boardstack_t	stack;

		for (const extmove_t *extmove = movelist_begin(&list);
			extmove < movelist_end(&list); ++extmove)
		{
			do_move(board, extmove->move, &stack);
			sum += perft(board, depth - 1);
			undo_move(board, extmove->move);
		}
		return (sum);
	}
}

void		*engine_go(void *ptr)
{
	extern goparams_t	g_goparams;
	extern ucioptions_t	g_options;
	extern movelist_t	g_searchmoves;
	const size_t		root_move_count = movelist_size(&g_searchmoves);
	board_t				*board = ptr;
	worker_t			*worker = get_worker(board);

	if (g_goparams.perft)
	{
		clock_t		time = chess_clock();
		uint64_t	nodes = perft(board, (unsigned int)g_goparams.perft);

		time = chess_clock() - time;

		uint64_t	nps = (!time) ? 0 : (nodes * 1000) / time;

		printf("info nodes %" FMT_INFO " nps %" FMT_INFO " time %" FMT_INFO "\n",
			(info_t)nodes, (info_t)nps, (info_t)time);

		return (NULL);
	}

	if (root_move_count == 0)
	{
		if (board->stack->checkers)
			printf("info depth 0 score mate 0\nbestmove 0000\n");
		else
			printf("info depth 0 score cp 0\nbestmove 0000\n");
		fflush(stdout);
		return (NULL);
	}

	// Init root move struct here

	root_move_t		*root_moves = malloc(sizeof(root_move_t) * root_move_count);
	for (size_t i = 0; i < movelist_size(&g_searchmoves); ++i)
	{
		root_moves[i].move = g_searchmoves.moves[i].move;
		root_moves[i].seldepth = 0;
		root_moves[i].previous_score = root_moves[i].score = -INF_SCORE;
		root_moves[i].pv[0] = NO_MOVE;
	}

	memset(worker->history, 0, sizeof(history_t));
	memset(worker->pawns_cache, 0,  sizeof(pawns_table_t));
	worker->verif_plies = 0;

	if (!worker->idx)
	{
		tt_clear();

		// Do we have to use the time manager ?

		if (g_goparams.wtime || g_goparams.btime || g_goparams.winc || g_goparams.binc)
		{
			if (g_goparams.movestogo == 0)
			{
				g_goparams.movestogo = 50;

				// If we have an increment to avoid time burns, decrease
				// estimated movestogo.

				if (board->side_to_move == WHITE && g_goparams.winc)
					g_goparams.movestogo = 35;
				if (board->side_to_move == BLACK && g_goparams.binc)
					g_goparams.movestogo = 35;
			}

			clock_t		our_time = (board->side_to_move == WHITE) ? g_goparams.wtime
				: g_goparams.btime;
			clock_t		our_inc = (board->side_to_move == WHITE) ? g_goparams.winc
				: g_goparams.binc;

			// Remove overhead from initial time

			our_time = max(0, our_time - g_options.move_overhead);

			clock_t		estimated_time = our_time / g_goparams.movestogo + our_inc;

			g_goparams.maximal_time = estimated_time * g_options.burn_ratio;
			g_goparams.optimal_time = estimated_time / g_options.save_ratio;

			g_goparams.maximal_time = min(g_goparams.maximal_time, our_time);
			g_goparams.optimal_time = min(g_goparams.optimal_time, our_time);
		}
		else if (g_goparams.movetime)
		{
			g_goparams.maximal_time = max(1, g_goparams.movetime - g_options.move_overhead);
			g_goparams.optimal_time = g_goparams.maximal_time;
		}
		else
			g_goparams.maximal_time = g_goparams.optimal_time = 0;

		if (g_goparams.depth == 0)
			g_goparams.depth = 240;

		if (g_goparams.nodes == 0)
			g_goparams.nodes = SIZE_MAX;

		WPool.checks = 1000;
		worker->nodes = 0;

		for (int i = 1; i < WPool.size; ++i)
		{
			worker_t	*cur = WPool.list + i;

			cur->board = worker->board;
			cur->stack = boardstack_dup(worker->stack);
			cur->board.stack = cur->stack;
			cur->board.worker = cur;
			cur->nodes = 0;

			if (pthread_create(&cur->thread, NULL, &engine_go, &cur->board))
			{
				perror("Unable to initialize worker thread");
				exit(EXIT_FAILURE);
			}
		}
	}

	const int	multi_pv = min(g_options.multi_pv, root_move_count);

	for (int iter_depth = 0; iter_depth < g_goparams.depth; ++iter_depth)
	{
		bool		has_search_aborted = false;

		for (int pv_line = 0; pv_line < multi_pv; ++pv_line)
		{
			worker->seldepth = 0;

			score_t	_alpha, _beta, _delta;

			if (iter_depth <= 9)
			{
				_delta = 0;
				_alpha = -INF_SCORE;
				_beta = INF_SCORE;
			}
			else
			{
				_delta = max(20, 160 / sqrt(iter_depth));
				_alpha = max(-INF_SCORE, root_moves[pv_line].previous_score - _delta);
				_beta = min(INF_SCORE, root_moves[pv_line].previous_score + _delta);
			}

__retry:

			search_bestmove(board, iter_depth + 1, _alpha, _beta,
				root_moves + pv_line, root_moves + root_move_count, pv_line);

			// Catch search aborting

			has_search_aborted = (g_engine_send == DO_ABORT || g_engine_send == DO_EXIT);

			sort_root_moves(root_moves + pv_line, root_moves + root_move_count);

			int		bound = (abs(root_moves[pv_line].score) == INF_SCORE) ? EXACT_BOUND
				: (root_moves[pv_line].score >= _beta) ? LOWER_BOUND
				: (root_moves[pv_line].score <= _alpha) ? UPPER_BOUND : EXACT_BOUND;

			if (bound == EXACT_BOUND)
				sort_root_moves(root_moves, root_moves + multi_pv);

			if (!worker->idx)
			{
				clock_t		chess_time = chess_clock() - g_goparams.start;
				uint64_t	chess_nodes = get_node_count();
				uint64_t	chess_nps = (!chess_time) ? 0 : (chess_nodes * 1000)
					/ chess_time;

				// Don't update Multi-PV lines if not all analysed at current depth
				// and not enough time elapsed

				if ((multi_pv == 1 && (bound == EXACT_BOUND || chess_time > 3000))
					|| (multi_pv > 1 && bound == EXACT_BOUND
						&& (pv_line == multi_pv - 1 || chess_time > 3000)))
				{
					for (int i = 0; i < multi_pv; ++i)
					{
						bool		searched = (root_moves[i].score != -INF_SCORE);
						score_t		root_score = (searched) ? root_moves[i].score
						: root_moves[i].previous_score;

						printf("info depth %d seldepth %d multipv %d nodes %" FMT_INFO
							" nps %" FMT_INFO " hashfull %d time %" FMT_INFO " score %s%s pv",
							max(iter_depth + (int)searched, 1), root_moves[i].seldepth, i + 1,
							(info_t)chess_nodes, (info_t)chess_nps,
							tt_hashfull(), (info_t)chess_time,
							score_to_str(root_score), bound == EXACT_BOUND ? ""
							: bound == LOWER_BOUND ? " lowerbound" : " upperbound");
	
						for (size_t k = 0; root_moves[i].pv[k] != NO_MOVE; ++k)
							printf(" %s", move_to_str(root_moves[i].pv[k],
								board->chess960));
						puts("");
					}
					fflush(stdout);
				}
			}

			if (has_search_aborted)
				break ;

			if (bound == UPPER_BOUND)
			{
				_beta = (_alpha + _beta) / 2;
				_alpha = max(-INF_SCORE, (int)_alpha - _delta);
				_delta += _delta / 4;
				goto __retry;
			}
			else if (bound == LOWER_BOUND)
			{
				_beta = min(INF_SCORE, (int)_beta + _delta);
				_delta += _delta / 4;
				goto __retry;
			}
		}

		for (root_move_t *i = root_moves; i < root_moves + root_move_count; ++i)
		{
			i->previous_score = i->score;
			i->score = -INF_SCORE;
		}

		if (has_search_aborted)
			break ;

		// If we went over optimal time usage, we just finished our iteration,
		// so we can safely return our bestmove.

		if (!worker->idx)
			if ((g_goparams.wtime || g_goparams.btime || g_goparams.winc || g_goparams.binc)
				&& chess_clock() - g_goparams.start >= g_goparams.optimal_time)
				break ;

		if (g_goparams.mate < 0 && root_moves->previous_score <= mated_in(1 - g_goparams.mate * 2))
			break ;
		if (g_goparams.mate > 0 && root_moves->previous_score >= mate_in(g_goparams.mate * 2))
			break ;
	}

	// UCI protocol specifies that we shouldn't send the bestmove command
	// before the GUI sends us the "stop" in infinite mode.

	if (g_goparams.infinite)
		while (!(g_engine_send == DO_ABORT || g_engine_send == DO_EXIT))
			if (!worker->idx)
				check_time();

	if (!worker->idx)
	{
		printf("bestmove %s\n", move_to_str(root_moves->move, board->chess960));
		fflush(stdout);

		if (g_engine_send != DO_ABORT)
		{
			pthread_mutex_lock(&g_engine_mutex);
			g_engine_send = DO_EXIT;
			pthread_mutex_unlock(&g_engine_mutex);

			for (int i = 1; i < WPool.size; ++i)
				pthread_join(WPool.list[i].thread, NULL);
		}
	}

	free(root_moves);
	boardstack_free(worker->stack);
	return (NULL);
}
