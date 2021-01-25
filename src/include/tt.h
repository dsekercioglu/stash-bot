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

#ifndef TT_H
# define TT_H

# include <string.h>
# include "score.h"
# include "hashkey.h"
# include "move.h"

typedef struct
{
    hashkey_t   key;
    score_t     score;
    score_t     eval;
    uint8_t     depth;
    uint8_t     genbound;
    uint16_t    bestmove;
}        tt_entry_t;

enum
{
    ClusterSize = 4
};

typedef tt_entry_t  cluster_t[ClusterSize];

typedef struct
{
    size_t      cluster_count;
    cluster_t   *table;
    uint8_t     generation;
}        transposition_t;

extern transposition_t  TT;

INLINED tt_entry_t  *tt_entry_at(hashkey_t k)
{
    return (TT.table[mul_hi64(k, TT.cluster_count)]);
}

INLINED void        tt_clear(void)
{
    TT.generation += 4;
}

INLINED void        tt_bzero(void)
{
    tt_entry_t  zero_entry = {0, NO_SCORE, NO_SCORE, 0, 0, NO_MOVE};
    TT.generation = 0;

    for (size_t i = 0; i < TT.cluster_count; ++i)
        for (size_t k = 0; k < ClusterSize; ++k)
            TT.table[i][k] = zero_entry;
}

INLINED score_t     score_to_tt(score_t s, int plies)
{
    return (s >= MATE_FOUND ? s + plies : s <= -MATE_FOUND ? s - plies : s);
}

INLINED score_t     score_from_tt(score_t s, int plies)
{
    return (s >= MATE_FOUND ? s - plies : s <= -MATE_FOUND ? s + plies : s);
}

tt_entry_t  *tt_probe(hashkey_t key, bool *found);
void        tt_save(tt_entry_t *entry, hashkey_t k, score_t s, score_t e, int d, int b, move_t m);
int         tt_hashfull(void);
void        tt_resize(size_t mbsize);

#endif
