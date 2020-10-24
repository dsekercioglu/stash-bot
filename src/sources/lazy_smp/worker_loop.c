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

#include "engine.h"
#include "lazy_smp.h"
#include <stdio.h>

void	*worker_loop(void *ptr)
{
	worker_t	*worker = ptr;

	pthread_mutex_lock(&worker->mutex);
	worker->mode = WAITING;
	pthread_cond_broadcast(&worker->cond);

	while (WPool.send != DO_ABORT)
	{
		pthread_cond_wait(&worker->cond, &worker->mutex);

		if (WPool.send == DO_THINK)
		{
			pthread_mutex_unlock(&worker->mutex);
			engine_go(&worker->board);
			pthread_mutex_lock(&worker->mutex);
			worker->mode = WAITING;

			if (is_main_worker(worker))
			{
				for (size_t i = 1; i < WPool.wcount; ++i)
				{
					worker_t	*w = &WPool.workers[i];
					pthread_mutex_lock(&w->mutex);

					while (w->mode != WAITING)
						pthread_cond_wait(&w->cond, &w->mutex);

					pthread_mutex_unlock(&w->mutex);
				}

				if (WPool.send != DO_ABORT)
					WPool.send = DO_NOTHING;
			}
			pthread_cond_broadcast(&worker->cond);
		}
	}

	pthread_mutex_unlock(&worker->mutex);
	return (NULL);
}
