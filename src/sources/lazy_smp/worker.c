#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lazy_smp.h"

void	init_worker(worker_t *worker)
{
	worker->mode = THINKING;
	if (pthread_create(&worker->thread, NULL, &worker_loop, worker))
	{
		perror("Unable to initialize worker");
		exit(EXIT_FAILURE);
	}
	pthread_mutex_init(&worker->mutex, NULL);
	pthread_cond_init(&worker->cond, NULL);
	worker->stack = NULL;
}

void	quit_worker(worker_t *worker)
{
	pthread_join(worker->thread, NULL);
	pthread_mutex_destroy(&worker->mutex);
	pthread_cond_destroy(&worker->cond);
	boardstack_free(worker->stack);
}

void	worker_reset(worker_t *worker)
{
	memset(worker->good_hist, 0, sizeof(history_t));
	memset(worker->bad_hist, 0, sizeof(history_t));
	memset(worker->pawn_table, 0, sizeof(pawns_table_t));
	worker->nodes = 0;
	worker->nmp_verif_plies = 0;
}
