/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2021 Morgan Houppin
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

#include <math.h>
#include <stdlib.h>
#include "imath.h"
#include "timeman.h"
#include "movelist.h"

double  score_difference_scale(score_t s)
{
    const score_t   X = 100;
    const double    T = 1.6;

    // Clamp score to the range [-100, 100], and convert it to a time scale [0.625, 1.6]
    // Examples:
    // -100 -> 1.600x time
    //  -50 -> 1.265x time
    //    0 -> 1.000x time
    //  +50 -> 0.791x time
    // +100 -> 0.625x time

    return (pow(T, clamp(s, -X, X) / (double)X));
}

void    timeman_update(timeman_t *tm, const board_t *board, move_t bestmove, score_t score)
{
    // Only update timeman when we need one
    if (tm->mode != Tournament)
        return ;

    // Update stability value
    if (tm->prev_bestmove != bestmove)
    {
        tm->prev_bestmove = bestmove;
        tm->stability = 0;
    }
    else
        tm->stability += 1;

    // Scale the time usage based on how long this bestmove has held
    // through search iterations
    double  scale = fmax(0.5, 1.4 * pow(0.95, tm->stability));

    // Scale time if there's only one legal move
    {
        movelist_t  list;

        list_all(&list, board);
        if (movelist_size(&list) == 1)
            scale /= 5.0;
    }

    // Scale the time usage based on how the score changed from the
    // previous iteration (the higher it goes, the quicker we stop searching)
    if (tm->prev_score != NO_SCORE)
        scale *= score_difference_scale(tm->prev_score - score);

    // Update score + optimal time usage
    tm->prev_score = score;
    tm->optimal_time = min(tm->maximal_time, tm->average_time * scale);
}
