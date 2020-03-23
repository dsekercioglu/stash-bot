/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   search_bestmove.c                                .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/23 22:01:23 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/03/11 12:25:01 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include "engine.h"
#include "info.h"
#include "movelist.h"
#include "tt.h"
#include "uci.h"

int		g_seldepth;

score_t	qsearch(board_t *board, int max_depth, score_t alpha, score_t beta,
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

	if (g_seldepth < cur_depth + 1)
		g_seldepth = cur_depth + 1;

	list_instable(&list, board);

	score_t	next = evaluate(board);

	if (alpha < next)
		alpha = next;

	if (movelist_size(&list) == 0)
	{
		if (board->stack->checkers)
			return (mated_in(cur_depth + 1));
		else
			return (alpha);
	}

	if (is_draw(board, cur_depth + 1))
		return (0);

	if (alpha >= beta)
		return (alpha);

	generate_move_values(&list, board, NO_MOVE);
	sort_moves((extmove_t *)movelist_begin(&list),
		(extmove_t *)movelist_end(&list));

	for (const extmove_t *extmove = movelist_begin(&list);
		extmove < movelist_end(&list); ++extmove)
	{
		boardstack_t	stack;

		do_move(board, extmove->move, &stack);

		score_t		next = -qsearch(board, max_depth - 1, -beta, -alpha,
			end, cur_depth + 1);

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

score_t	search(board_t *board, int max_depth, score_t alpha, score_t beta,
		clock_t end, int cur_depth)
{
	if (max_depth <= 0)
		return (qsearch(board, max_depth, alpha, beta, end, cur_depth));

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

	if (g_seldepth < cur_depth + 1)
		g_seldepth = cur_depth + 1;

	// Mate pruning.

	{
		score_t		mate_alpha = mated_in(cur_depth);
		score_t		mate_beta = mate_in(cur_depth + 1);

		if (alpha < mate_alpha)
			alpha = mate_alpha;
		if (beta > mate_beta)
			beta = mate_beta;

		if (alpha >= beta)
			return (alpha);
	}

	list_all(&list, board);

	if (movelist_size(&list) == 0)
	{
		if (board->stack->checkers)
			return (mated_in(cur_depth));
		else
			return (0);
	}

	if (is_draw(board, cur_depth + 1))
		return (0);

	// Check for interesting tt values

	move_t		tt_move = NO_MOVE;
	bool		found;
	tt_entry_t	*entry = tt_probe(board->stack->board_key, &found);

	if (found)
	{
		extern transposition_t	g_hashtable;

		// Do not trust past entries, because of possible TT collisions
		// or 3-fold blindness

		if (entry->depth >= max_depth - DEPTH_OFFSET
			&& (entry->genbound & 0xFC) == g_hashtable.generation)
		{
			int	bound = entry->genbound & 3;

			if (bound == EXACT_BOUND)
				return (entry->score);
			else if (bound == LOWER_BOUND && entry->score > alpha)
			{
				alpha = entry->score;
				if (alpha >= beta)
					return (alpha);
			}
			else if (bound == UPPER_BOUND && entry->score < beta)
			{
				beta = entry->score;
				if (alpha >= beta)
					return (alpha);
			}
		}
		tt_move = entry->bestmove;
	}

	// Null move pruning.

	if (max_depth >= 2 && !board->stack->checkers
		&& board->stack->plies_from_null_move >= 3)
	{
		boardstack_t	stack;

		do_null_move(board, &stack);

		score_t			score = -search(board, max_depth - 3 - (max_depth / 4), -beta, -beta + 1,
			end, cur_depth);

		undo_null_move(board);

		if (abs(score) > INF_SCORE)
			return (NO_SCORE);

		if (score >= beta)
		{
			// Do not trust mate claims.

			if (score > MATE_FOUND)
				score = beta;

			// Do not trust win claims.

			if (max_depth < 11 && beta < 5000)
				return (score);

			// Zugzwang checking.

			int nmp_depth = board->stack->plies_from_null_move;
			board->stack->plies_from_null_move = 0;

			score_t		zzscore = search(board, max_depth - 3 - (max_depth / 4), beta - 1, beta,
				end, cur_depth);

			board->stack->plies_from_null_move = nmp_depth;

			if (zzscore >= beta)
				return (score);
		}
	}

	generate_move_values(&list, board, tt_move);
	sort_moves((extmove_t *)movelist_begin(&list),
		(extmove_t *)movelist_end(&list));

	move_t	bestmove = NO_MOVE;

	for (const extmove_t *extmove = movelist_begin(&list);
		extmove < movelist_end(&list); ++extmove)
	{
		boardstack_t	stack;

		do_move(board, extmove->move, &stack);

		score_t		next;

		if (extmove == movelist_begin(&list))
			next = -search(board, max_depth - 1, -beta, -alpha,
				end, cur_depth + 1);
		else
		{
			int		new_depth = max_depth - 1;

			if (max_depth < 3 && extmove >= movelist_begin(&list) + 4
				&& !board->stack->checkers)
				new_depth -= 1;

			next = -search(board, new_depth, -alpha - 1, -alpha,
				end, cur_depth + 1);

			if (alpha < next && next < beta)
				next = -search(board, max_depth - 1, -beta, -next,
					end, cur_depth + 1);
		}

		undo_move(board, extmove->move);

		if (abs(next) > INF_SCORE)
		{
			alpha = NO_SCORE;
			break ;
		}

		if (alpha < next)
		{
			bestmove = extmove->move;
			alpha = next;
		}

		if (alpha >= beta)
			break ;
	}

	if (alpha != NO_SCORE)
	{
		int bound = (bestmove == NO_MOVE) ? UPPER_BOUND
			: (alpha >= beta) ? LOWER_BOUND : EXACT_BOUND;

		tt_save(entry, board->stack->board_key, alpha, max_depth, bound, bestmove);
	}

	return (alpha);
}

void	search_bestmove(board_t *board, int depth, size_t pv_line,
		clock_t start)
{
	extern movelist_t	g_searchmoves;
	extern goparams_t	g_goparams;
	score_t				alpha = -INF_SCORE;

	g_seldepth = 0;

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
			g_searchmoves.moves[i].score = -search(board, depth, -INF_SCORE,
				INF_SCORE, end, 1);
		else
		{
			g_searchmoves.moves[i].score = -search(board, depth, -alpha - 1,
				-alpha, end, 1);

			if (alpha < g_searchmoves.moves[i].score)
				g_searchmoves.moves[i].score = -search(board, depth,
					-INF_SCORE, -g_searchmoves.moves[i].score, end, 1);
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
