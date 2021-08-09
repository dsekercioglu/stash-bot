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
#include "engine.h"
#include "imath.h"
#include "lazy_smp.h"
#include "timeman.h"

void timeman_init(const board_t *board, timeman_t *tm, goparams_t *params, clock_t start)
{
    clock_t overhead = Options.moveOverhead;

    tm->start = start;
    tm->pondering = false;

    if (params->wtime || params->btime)
    {
        tm->mode = Tournament;

        double mtg = (params->movestogo) ? params->movestogo : 40.0;
        clock_t time = (board->sideToMove == WHITE) ? params->wtime : params->btime;
        clock_t inc = (board->sideToMove == WHITE) ? params->winc : params->binc;

        time = max(0, time - overhead);

        tm->averageTime = time / mtg + inc;
        tm->maximalTime = time / pow(mtg, 0.37) + inc;

        // Allow for more time usage when we're pondering, since we're not using
        // our clock as long as the opponent thinks
        if (params->ponder)
        {
            tm->pondering = true;
            tm->averageTime += tm->averageTime / 4;
        }

        tm->averageTime = min(tm->averageTime, time);
        tm->maximalTime = min(tm->maximalTime, time);
        tm->optimalTime = tm->maximalTime;
    }
    else if (params->movetime)
    {
        tm->mode = Movetime;
        tm->averageTime = tm->maximalTime = tm->optimalTime = max(1, params->movetime - overhead);
    }
    else
        tm->mode = NoTimeman;
}

void timeman_update(timeman_t *tm, const root_move_t *rootMoves, size_t rootCount)
{
    // Only update timeman when we need one
    if (tm->mode != Tournament)
        return ;

    uint64_t bestNodes = rootMoves[0].nodes;
    uint64_t otherNodes = 0;

    for (size_t i = 1; i < rootCount; ++i)
        otherNodes += rootMoves[i].nodes;

    double scale = 3.85 * pow((double)otherNodes / (double)bestNodes, 1.12);

    scale = fmax(0.1, fmin(scale, 2.3));

    // Update optimal time usage
    tm->optimalTime = min(tm->maximalTime, tm->averageTime * scale);
}

void check_time(void)
{
    if (--WPool.checks > 0)
        return ;

    // Reset check counter

    WPool.checks = 1000;

    // If we are in infinite mode, or the stop has already been set,
    // we can safely return.

    if (SearchParams.infinite || search_should_abort())
        return ;

    if (get_node_count() >= SearchParams.nodes)
        goto __set_stop;

    if (timeman_must_stop_search(&Timeman, chess_clock()))
        goto __set_stop;

    return ;

__set_stop:
    pthread_mutex_lock(&EngineMutex);
    EngineSend = DO_EXIT;
    pthread_mutex_unlock(&EngineMutex);
}
