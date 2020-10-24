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

#include "lazy_smp.h"

void	wait_search_end(void)
{
	for (size_t i = 0; i < WPool.wcount; ++i)
	{
		worker_t	*worker = &WPool.workers[i];

		pthread_mutex_lock(&worker->mutex);

		while (worker->mode != WAITING)
			pthread_cond_wait(&worker->cond, &worker->mutex);

		pthread_mutex_unlock(&worker->mutex);
	}
}
