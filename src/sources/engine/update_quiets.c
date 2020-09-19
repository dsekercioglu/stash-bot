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
#include "history.h"

void	update_quiets(const board_t *board, move_t bestmove,
		const move_t *quiets, int quiets_searched, int depth,
		searchstack_t *ss)
{
	int		hist_bonus = depth * depth;

	if (!ss->killers[0])
		ss->killers[0] = bestmove;
	else if (ss->killers[0] != bestmove)
		ss->killers[1] = bestmove;

	add_hist_bonus(piece_on(board, move_from_square(bestmove)),
		bestmove, hist_bonus);

	for (int i = 0; i < quiets_searched; i++)
	{
		add_hist_penalty(piece_on(board, move_from_square(quiets[i])),
			quiets[i], hist_bonus);
	}
}
