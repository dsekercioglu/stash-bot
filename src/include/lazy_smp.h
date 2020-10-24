#ifndef LAZY_SMP_H
# define LAZY_SMP_H

# include <pthread.h>
# include "board.h"
# include "history.h"
# include "pawns.h"

typedef enum
{
	WAITING,
	THINKING
}
worker_mode_t;

typedef enum
{
	DO_NOTHING,
	DO_THINK,
	DO_EXIT,
	DO_ABORT
}
worker_send_t;

typedef struct
{
	board_t			board;
	boardstack_t	*stack;
	history_t		good_hist;
	history_t		bad_hist;
	pawns_table_t	pawn_table;

	pthread_t		thread;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;

	// Search stats

	int					seldepth;
	int					nmp_verif_plies;
	_Atomic uint64_t	nodes;

	volatile worker_mode_t	mode;
}
worker_t;

typedef struct
{
	size_t			wcount;
	worker_t		*workers;
	worker_send_t	send;
}
worker_pool_t;

extern worker_pool_t	WPool;

INLINED worker_t	*get_worker(const board_t *board)
{
	return (board->worker);
}

INLINED bool		is_main_worker(worker_t *worker)
{
	return (worker == WPool.workers);
}

INLINED uint64_t	get_node_count(void)
{
	uint64_t	count = 0;

	for (size_t i = 0; i < WPool.wcount; ++i)
		count += WPool.workers[i].nodes;

	return (count);
}

void	init_wpool(size_t count);
void	init_worker(worker_t *worker);
void	quit_wpool(void);
void	quit_worker(worker_t *worker);
void	wait_search_end(void);
void	*worker_loop(void *ptr);
void	worker_reset(worker_t *worker);

#endif
