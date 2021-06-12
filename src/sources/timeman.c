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
        tm->maximalTime = time / sqrt(mtg) + inc;

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

    tm->prevScore = NO_SCORE;
    tm->prevBestmove = NO_MOVE;
    tm->stability = 0;
}

double score_difference_scale(score_t s)
{
    const score_t X = 100;
    const double T = 2.0;

    // Clamp score to the range [-100, 100], and convert it to a time scale [0.5, 2.0]
    // Examples:
    // -100 -> 2.000x time
    //  -50 -> 1.414x time
    //    0 -> 1.000x time
    //  +50 -> 0.707x time
    // +100 -> 0.500x time

    return (pow(T, clamp(s, -X, X) / (double)X));
}

double bestmove_stability_scale(int stab)
{
    const double U = 2.5;
    const double L = 0.8;
    const double P = 0.5;

    return fmax(L, U / pow(stab + 1, P));
}

void timeman_update(timeman_t *tm, const board_t *board, move_t bestmove, score_t score)
{
    // Only update timeman when we need one
    if (tm->mode != Tournament)
        return ;

    double scale = 1.0;

    // Update bestmove + stability statistics
    if (tm->prevBestmove != bestmove)
    {
        tm->prevBestmove = bestmove;
        tm->stability = 0;

        // Do we only have one legal move ? Don't burn much time on these
        movelist_t list;

        list_all(&list, board);
        if (movelist_size(&list) == 1)
            scale = 0.2;
    }
    else
        tm->stability++;

    // Scale the time usage based on how long this bestmove has held
    // through search iterations
    scale *= bestmove_stability_scale(tm->stability);

    // Scale the time usage based on how the score changed from the
    // previous iteration (the higher it goes, the quicker we stop searching)
    if (tm->prevScore != NO_SCORE)
        scale *= score_difference_scale(tm->prevScore - score);

    // Update score + optimal time usage
    tm->prevScore = score;
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
