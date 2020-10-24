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

#ifndef PAWNS_H
# define PAWNS_H

# include "board.h"

typedef struct	pawns_cache_s
{
	hashkey_t	key;
	scorepair_t	value;
}
pawns_cache_t;

enum
{
	PawnCacheSize = 8192
};

typedef pawns_cache_t	pawns_table_t[PawnCacheSize];

scorepair_t		evaluate_pawns(const board_t *board);

#endif
