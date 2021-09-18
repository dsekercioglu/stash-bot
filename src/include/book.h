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

#ifndef BOOK_H
# define BOOK_H

# include <stddef.h>
# include "hashkey.h"
# include "move.h"
# include "score.h"

typedef struct book_entry_s
{
    hashkey_t key;
    uint32_t kNodes;
    score_t score;
    uint8_t prevCount;
    uint8_t nextCount;
    hashkey_t *prevKeys;
    hashkey_t *nextKeys;
    move_t *nextMoves;
}
book_entry_t;

typedef struct book_s
{
    size_t size;
    size_t maxSize;
    book_entry_t *entries;
}
book_t;

extern book_t Book;

int book_load(book_t *book, const char *filename);
int book_save(const book_t *book, const char *filename);
int book_update_entry(book_t *book, hashkey_t key, hashkey_t nextKey,
    uint32_t kNodes, uint32_t nextKNodes, move_t bestmove, score_t score);
void book_probe(const book_t *book, hashkey_t key, score_t *score, move_t *move, uint32_t *kNodes, double R);
void book_free(book_t *book);
void book_tree(const book_t *book, hashkey_t key, int depth);

#endif
