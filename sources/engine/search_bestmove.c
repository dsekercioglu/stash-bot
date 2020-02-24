/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   search_bestmove.c                                .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 22:01:23 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 14:14:46 by mhouppin         ###   ########lyon.fr   */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include <stdio.h>
#include "engine.h"
#include "info.h"
#include "movelist.h"
#include "uci.h"

score_t	alpha_beta(board_t *board, int max_depth, score_t alpha, score_t beta,
		clock_t end, int cur_depth)
{
	extern goparams_t	g_goparams;
	movelist_t			list;

	if (g_nodes >= g_goparams.nodes)
		return (NO_SCORE);

	if (g_nodes % 16384 == 0)
	{
		if (!g_goparams.infinite && chess_clock() > end)
			return (NO_SCORE);
		else
		{
			pthread_mutex_lock(&g_engine_mutex);

			if (g_engine_send == DO_ABORT || g_engine_send == DO_EXIT)
			{
				pthread_mutex_unlock(&g_engine_mutex);
				return (NO_SCORE);
			}
			pthread_mutex_unlock(&g_engine_mutex);
		}
	}

	if (max_depth == 0)
		return (evaluate(board));

	list_all(&list, board);

	if (movelist_size(&list) == 0)
	{
		if (board->stack->checkers)
			return (mated_in(cur_depth + 1));
		else
			return (0);
	}

	if (is_draw(board, cur_depth + 1))
		return (0);

	if (max_depth > 1)
	{
		generate_move_values(&list, board);
		sort_moves((extmove_t *)movelist_begin(&list),
			(extmove_t *)movelist_end(&list));
	}

	for (const extmove_t *extmove = movelist_begin(&list);
		extmove < movelist_end(&list); ++extmove)
	{
		boardstack_t	stack;

		do_move(board, extmove->move, &stack);

		score_t		next;

		if (extmove == movelist_begin(&list))
			next = -alpha_beta(board, max_depth - 1, -beta, -alpha,
				end, cur_depth + 1);
		else
		{
			next = -alpha_beta(board, max_depth - 1, -alpha - 1, -alpha,
				end, cur_depth + 1);

			if (alpha < next && next < beta)
				next = -alpha_beta(board, max_depth - 1, -beta, -next,
					end, cur_depth + 1);
		}

		undo_move(board, extmove->move);

		if (abs(next) > INF_SCORE)
		{
			alpha = NO_SCORE;
			break ;
		}

		if (alpha < next)
			alpha = next;

		if (alpha >= beta)
			break ;
	}
	return (alpha);
}

void	search_bestmove(board_t *board, int depth, size_t pv_line,
		clock_t start)
{
	extern movelist_t	g_searchmoves;
	extern goparams_t	g_goparams;
	score_t				alpha = -INF_SCORE;

	for (size_t i = pv_line; i < movelist_size(&g_searchmoves); ++i)
	{
		pthread_mutex_lock(&g_engine_mutex);

		if (g_engine_send == DO_ABORT || g_engine_send == DO_EXIT)
		{
			g_searchmoves.moves[i].score = NO_SCORE;
			pthread_mutex_unlock(&g_engine_mutex);
			return ;
		}
		pthread_mutex_unlock(&g_engine_mutex);

		boardstack_t	stack;

		do_move(board, g_searchmoves.moves[i].move, &stack);

		clock_t			elapsed = chess_clock() - start;

		if (elapsed > 3000)
		{
			size_t	nps = g_nodes * 1000ul / (size_t)elapsed;

			printf("info depth %d nodes " SIZE_FORMAT " nps " SIZE_FORMAT
				" time %lu currmove %s currmovenumber " SIZE_FORMAT "\n",
				depth + 1, (size_t)g_nodes, nps, elapsed,
				move_to_str(g_searchmoves.moves[i].move, board->chess960),
				i + 1);
			fflush(stdout);
		}

		clock_t		end = start + (g_goparams.movetime);

		if (i == 0)
			g_searchmoves.moves[i].score = -alpha_beta(board, depth, -INF_SCORE,
				INF_SCORE, end, 0);
		else
		{
			g_searchmoves.moves[i].score = -alpha_beta(board, depth, -alpha - 1,
				-alpha, end, 0);

			if (alpha < g_searchmoves.moves[i].score)
				g_searchmoves.moves[i].score = -alpha_beta(board, depth,
					-INF_SCORE, -g_searchmoves.moves[i].score, end, 0);
		}

		undo_move(board, g_searchmoves.moves[i].move);

		if (abs(g_searchmoves.moves[i].score) > INF_SCORE)
			return ;
		else if (g_searchmoves.moves[i].score > alpha)
			alpha = g_searchmoves.moves[i].score;
		else if (g_searchmoves.moves[i].score == alpha)
			g_searchmoves.moves[i].score -= 1;
	}
	return ;
}