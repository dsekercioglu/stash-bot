/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2020 Morgan Houppin
**
**    Stash is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    Stash is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HISTORY_H
# define HISTORY_H

# include <stdlib.h>
# include "move.h"
# include "piece.h"
# include "score.h"

enum
{
    HistoryMaxScore = 512,
    HistoryScale = 16,
    HistoryResolution = HistoryMaxScore * HistoryScale
};

typedef int32_t qhistory_t[PIECE_NB][SQUARE_NB * SQUARE_NB];
typedef int32_t chistory_t[PIECE_NB][SQUARE_NB][PIECETYPE_NB];

INLINED int     history_bonus(int depth)
{
    return (depth <= 12 ? 16 * depth * depth : 20);
}

INLINED void    add_qhistory(qhistory_t hist, piece_t piece, move_t move, int32_t bonus)
{
    int32_t        *entry = &hist[piece][move_squares(move)];

    *entry += bonus - *entry * abs(bonus) / HistoryResolution;
}

INLINED score_t get_qhistory_score(qhistory_t hist, piece_t piece, move_t move)
{
    return (hist[piece][move_squares(move)] / HistoryScale);
}

INLINED void    add_chistory(chistory_t hist, piece_t piece, square_t to, piece_t captured, int32_t bonus)
{
    int32_t         *entry = &hist[piece][to][captured];

    *entry += bonus - *entry * abs(bonus) / HistoryResolution;
}

INLINED score_t get_chistory_score(chistory_t hist, piece_t piece, square_t to, piece_t captured)
{
    return (hist[piece][to][captured] / HistoryScale);
}

#endif
