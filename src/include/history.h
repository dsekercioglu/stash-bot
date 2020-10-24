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

#ifndef HISTORY_H
# define HISTORY_H

# include <string.h>
# include "move.h"
# include "piece.h"
# include "score.h"

enum
{
	HistoryBonus = 256
};

typedef uint64_t	history_t[PIECE_NB][SQUARE_NB * SQUARE_NB];

INLINED void	add_history(history_t hist, piece_t piece, move_t move)
{
	hist[piece][move_squares(move)] += 1;
}

INLINED score_t	get_history_score(history_t good_hist, history_t bad_hist,
				piece_t piece, move_t move)
{
	const int	idx = move_squares(move);

	// Small trick to catch zero-division error and history score overflow.

	if (good_hist[piece][idx] >= bad_hist[piece][idx])
		return (HistoryBonus);

	return (good_hist[piece][idx] * HistoryBonus / bad_hist[piece][idx]);
}

#endif
