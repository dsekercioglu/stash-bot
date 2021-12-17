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
# include "board.h"

typedef struct book_movedata_s
{
    move_t move;
    hashkey_t keyAfterMove;
    uint64_t nodes;
    score_t score;
}
book_movedata_t;

typedef struct book_next_s
{
    move_t move;
    hashkey_t key;
}
book_next_t;

typedef struct book_entry_s
{
    hashkey_t key;
    uint64_t nodes;
    score_t score;
    uint16_t count;
    uint16_t inBook;
    uint16_t prevCount;
    book_next_t *list;
    hashkey_t *prevList;
}
book_entry_t;

typedef struct book_s
{
    size_t size;
    size_t capacity;
    book_entry_t *list;
}
book_t;

extern book_t Book;

void book_init(book_t *book);
void book_destroy(book_t *book);

int book_load(book_t *book, const char *filename);
int book_save(book_t *book, const char *filename);

book_entry_t *book_probe(book_t *book, const board_t *board);
book_entry_t *book_probe_key(book_t *book, hashkey_t key);

int book_entry_select(book_t *book, const book_entry_t *entry, int variance, move_t *move, score_t *score);

int book_update(book_t *book, const board_t *board, size_t dataSize, const book_movedata_t *data);
int book_update_entry(book_t *book, hashkey_t nextKey, const board_t *board, move_t move, uint64_t nodes, score_t score);

void book_info(book_t *book, const board_t *board);

#endif
