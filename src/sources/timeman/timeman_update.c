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
/*
score_t TimemanX = 100;
double  TimemanT = 2.0;

double  TimemanStabMax = 1.4;
long    TimemanStabPly = 1;
double  TimemanStabPow = 0.05;
double  TimemanStabMin = 0.5;
*/

double  score_difference_scale(score_t s)
{
    const score_t   X = 114;
    const double    T = 5.59;

    // Clamp score to the range [-X, X], and convert it to a time scale [1/T, T]
    // Examples:
    //   -X -> T x time
    // -X/2 -> sqrt(T) x time
    //    0 -> 1 x time
    // +X/2 -> 1/sqrt(T) x time
    //   +X -> 1/T x time

    return (pow(T, clamp(s, -X, X) / (double)X));
}

void    timeman_update(timeman_t *tm, const board_t *board, move_t bestmove, score_t score)
{
    // Only update timeman when we need one
    if (tm->mode != Tournament)
        return ;

    // Update bestmove + stability statistics
    if (tm->prev_bestmove != bestmove)
    {
        tm->prev_bestmove = bestmove;
        tm->stability = 0;
    }
    else
        tm->stability++;

    // Scale the time usage based on how long this bestmove has held
    // through search iterations
    // double  scale = fmax(TimemanStabMax * pow(1.0 - TimemanStabPow, (tm->stability / TimemanStabPly) * TimemanStabPly), TimemanStabMin);
    double  scale = fmax(2.91 * pow(0.948, (tm->stability / 7) * 7), 0.69);

    // Do we only have one legal move ? Don't burn much time on these
    movelist_t  list;
    list_all(&list, board);
    if (movelist_size(&list) == 1)
        scale /= 5.0;

    // Scale the time usage based on how the score changed from the
    // previous iteration (the higher it goes, the quicker we stop searching)
    if (tm->prev_score != NO_SCORE)
        scale *= score_difference_scale(tm->prev_score - score);

    // Update score + optimal time usage
    tm->prev_score = score;
    tm->optimal_time = min(tm->maximal_time, tm->average_time * scale);
}
