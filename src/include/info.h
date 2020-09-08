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

#ifndef INFO_H
# define INFO_H

# include <stdint.h>
# include "board.h"
# include "move.h"
# include "movelist.h"

extern uint64_t		g_nodes;

const char	*move_to_str(move_t move, bool is_chess960);
const char	*score_to_str(score_t score);

typedef unsigned long	info_t;

move_t		str_to_move(const board_t *board, const char *str);

#endif
