#include <stdio.h>
#include <stdlib.h>
#include "lazy_smp.h"
#include "uci.h"

worker_pool_t	WPool = {
	0, NULL, 0
};

void	init_wpool(size_t count)
{
	quit_wpool();
	WPool.workers = malloc(sizeof(worker_t) * count);

	if (WPool.workers == NULL)
	{
		perror("Unable to initialize workers");
		exit(EXIT_FAILURE);
	}

	WPool.wcount = count;
	WPool.send = DO_NOTHING;
	for (size_t i = 0; i < count; ++i)
		init_worker(&WPool.workers[i]);

	wait_search_end();
}

void	quit_wpool(void)
{
	wait_search_end();
	uci_quit(NULL);

	for (size_t i = 0; i < WPool.wcount; ++i)
		quit_worker(&WPool.workers[i]);
	free(WPool.workers);
}
