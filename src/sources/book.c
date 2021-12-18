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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "book.h"
#include "movelist.h"
#include "random.h"
#include "timeman.h"

book_t Book;

// Small functions to handle machine endianness.
INLINED uint16_t book_get_u16(uint16_t v)
{
    uint8_t *p = (uint8_t *)&v;

    return ((uint16_t)p[0] << 8) | ((uint16_t)p[1]);
}

// Small functions to handle machine endianness.
INLINED uint64_t book_get_u64(uint64_t v)
{
    uint8_t *p = (uint8_t *)&v;

    return ((uint64_t)p[0] << 56)
         | ((uint64_t)p[1] << 48)
         | ((uint64_t)p[2] << 40)
         | ((uint64_t)p[3] << 32)
         | ((uint64_t)p[4] << 24)
         | ((uint64_t)p[5] << 16)
         | ((uint64_t)p[6] << 8)
         | ((uint64_t)p[7]);
}

#if SIZE_MAX == UINT64_MAX
# define book_get_z(x) book_get_u64(x)
#else
INLINED size_t book_get_z(size_t v)
{
    uint8_t *p = (uint8_t *)&v;

    return ((uint64_t)p[0] << 24)
         | ((uint64_t)p[1] << 16)
         | ((uint64_t)p[2] << 8)
         | ((uint64_t)p[3]);
}
#endif

// Standard binary search for finding the entry for a given key if it exists.
book_entry_t *book_probe_key(book_t *book, hashkey_t key)
{
    size_t left = 0;
    size_t right = book->size;

    while (left < right)
    {
        size_t index = (left + right) / 2;
        book_entry_t *cur = book->list + index;

        if (key < cur->key)
            right = index;
        else if (key > cur->key)
            left = index + 1;
        else
            return (cur);
    }

    return (NULL);
}

book_entry_t *book_probe(book_t *book, const board_t *board)
{
    return book_probe_key(book, board->stack->boardKey);
}

int book_entry_select(book_t *book, const book_entry_t *entry, int variance, move_t *move, score_t *score)
{
    qseed(chess_clock());

    if (entry->inBook == 0)
        return (-1);

    double winrate[256];
    uint16_t topIndex = 0;
    score_t topScore = -INF_SCORE;

    for (uint16_t i = 0; i < entry->inBook; ++i)
    {
        book_entry_t *nextEntry = book_probe_key(book, entry->list[i].key);

        if (topScore < -nextEntry->score)
        {
            topScore = -nextEntry->score;
            topIndex = i;
        }

        winrate[i] = -nextEntry->score > MATE_FOUND ? 1.0
            : -nextEntry->score < -MATE_FOUND ? 0.0
            : 1.0 / (1.0 + exp(nextEntry->score / 200.0));
    }

    {
        book_entry_t *nextEntry = book_probe_key(book, entry->list[topIndex].key);

        *move = entry->list[topIndex].move;
        *score = -nextEntry->score;
    }

    if (topScore <= -MATE_FOUND)
        return 0;

    double probSum = 0.0;

    for (uint16_t i = 0; i < entry->inBook; ++i)
    {
        winrate[i] = pow(winrate[i], 1.0 - fmin(1.0, variance / 100.0));

        probSum += winrate[i];
    }

    double r = (double)(qrandom() & 0xFFFFu) * probSum / 65536.0;

    for (uint16_t i = 0; i < entry->inBook; ++i)
    {
        r -= winrate[i];

        if (r <= 0.0)
        {
            book_entry_t *nextEntry = book_probe_key(book, entry->list[i].key);

            *move = entry->list[i].move;
            *score = -nextEntry->score;
            return 0;
        }
    }

    return 0;
}

// Use a standard binary search for finding the position of the entry to
// insert, and insert it. If an allocation fails, return NULL. (Note that we)
// keep the book in a valid state in case of an error.
book_entry_t *book_add_entry(book_t *book, const board_t *board, uint64_t nodes, score_t score)
{
    if (book->size == book->capacity)
    {
        size_t newSize = book->capacity + (book->capacity / 2) + 1;
        void *ptr = realloc(book->list, sizeof(book_entry_t) * newSize);

        if (ptr == NULL)
            return (NULL);

        book->list = ptr;
        book->capacity = newSize;
    }

    size_t left = 0;
    size_t right = book->size;

    while (left < right)
    {
        size_t index = (left + right) / 2;
        book_entry_t *cur = book->list + index;

        if (board->stack->boardKey < cur->key)
            right = index;
        else
            left = index + 1;
    }

    uint16_t count;
    {
        movelist_t list;
        list_all(&list, board);
        count = movelist_size(&list);
    }

    // Shift all entries after the insertion to the right.
    memmove(book->list + left + 1, book->list + left, (book->size - left) * sizeof(book_entry_t));

    // Then write the entry data in the list.
    book->list[left] = (book_entry_t){board->stack->boardKey, nodes, -score, count, 0, 0, NULL, NULL};
    ++book->size;

    return (book->list + left);
}

book_next_t *book_entry_probe_move(book_entry_t *entry, move_t move)
{
    for (uint16_t i = 0; i < entry->inBook; ++i)
        if (entry->list[i].move == move)
            return (entry->list + i);

    return (NULL);
}

int book_entry_add_move(book_entry_t *entry, book_entry_t *nextEntry, move_t move)
{
    void *ptr = realloc(entry->list, sizeof(book_next_t) * (entry->inBook + 1));
    if (ptr == NULL)
        return (-1);

    entry->list = ptr;

    ptr = realloc(nextEntry->prevList, sizeof(hashkey_t) * (nextEntry->prevCount + 1));
    if (ptr == NULL)
        return (-1);

    nextEntry->prevList = ptr;

    entry->list[entry->inBook++] = (book_next_t){move, nextEntry->key};
    nextEntry->prevList[nextEntry->prevCount++] = entry->key;
    return (0);
}

int book_update_entry(book_t *book, hashkey_t nextKey, const board_t *board, move_t move, uint64_t nodes, score_t score)
{
    hashkey_t key = board->stack->boardKey;

    book_entry_t *nextEntry = book_probe_key(book, nextKey);

    // If the entry specific to the move doesn't exist, add it.
    // Otherwise check if we have more precise data by comparing the node count
    // in the entry to the node count of the current search.
    if (nextEntry == NULL)
    {
        // Small hack to bypass the const state of the board.
        board_t copyBoard = *board;
        boardstack_t stack;

        do_move(&copyBoard, move, &stack);
        nextEntry = book_add_entry(book, &copyBoard, nodes, score);
        undo_move(&copyBoard, move);

        if (nextEntry == NULL)
            return (-1);
    }
    else if (nextEntry->nodes < nodes)
    {
        nextEntry->nodes = nodes;
        nextEntry->score = -score;
    }

    book_entry_t *rootEntry = book_probe_key(book, key);

    // If the root entry doesn't exist, add it.
    // Either case make sure that the root entry is correctly linked to the
    // current move in both directions.
    if (rootEntry != NULL)
    {
        book_next_t *next = book_entry_probe_move(rootEntry, move);

        if (next == NULL && book_entry_add_move(rootEntry, nextEntry, move))
            return (-1);
    }
    else
    {
        rootEntry = book_add_entry(book, board, 0, NO_SCORE);
        nextEntry = book_probe_key(book, nextKey);

        if (rootEntry == NULL || book_entry_add_move(rootEntry, nextEntry, move))
            return (-1);
    }

    return (0);
}

void book_backpropagate(book_t *book, hashkey_t key)
{
    book_entry_t *entry = book_probe_key(book, key);

    if (entry->nodes == UINT64_MAX)
        return ;

    uint64_t maxNodes = 0;
    score_t maxScore = -INF_SCORE;

    for (uint16_t i = 0; i < entry->inBook; ++i)
    {
        book_entry_t *nextEntry = book_probe_key(book, entry->list[i].key);

        maxNodes = nextEntry->nodes == UINT64_MAX || maxNodes < nextEntry->nodes ? maxNodes : nextEntry->nodes;
        maxScore = max(maxScore, -nextEntry->score);
    }

    // Only backpropagate the score if we have enough total nodes and we're not
    // reporting a fake mate.
    if (maxNodes >= entry->nodes && (maxScore > -MATE_FOUND || entry->inBook == entry->count))
    {
        // To avoid infinite recursion
        entry->nodes = UINT64_MAX;
        entry->score = maxScore;

        for (uint16_t i = 0; i < entry->prevCount; ++i)
            book_backpropagate(book, entry->prevList[i]);

        entry->nodes = maxNodes;
    }
}

int book_update(book_t *book, const board_t *board, size_t dataSize, const book_movedata_t *data)
{
    for (size_t i = 0; i < dataSize; ++i)
    {
        const book_movedata_t *cur = data + i;

        if (book_update_entry(book, cur->keyAfterMove, board, cur->move, cur->nodes, cur->score))
            return (-1);
    }

    // Backpropagate the score values in a similar way to a minimax search.
    book_backpropagate(book, board->stack->boardKey);

    return (0);
}


void book_init(book_t *book)
{
    *book = (book_t){0, 0, NULL};
}

void book_destroy(book_t *book)
{
    for (size_t i = 0; i < book->size; ++i)
    {
        book_entry_t *entry = book->list + i;

        free(entry->list);
        free(entry->prevList);
    }

    free(book->list);
    book_init(book);
}

int book_load(book_t *book, const char *filename)
{
    book_destroy(book);
    FILE *f = fopen(filename, "r");

    if (f == NULL)
        return (-1);

    if (fread(&book->capacity, sizeof(size_t), 1, f) != 1)
    {
        fclose(f);
        return (-1);
    }

    book->capacity = book_get_z(book->capacity);
    book->list = calloc(book->capacity, sizeof(book_entry_t));

    if (book->list == NULL)
    {
        book->capacity = 0;
        fclose(f);
        return (-1);
    }

    for (book->size = 0; book->size < book->capacity; ++book->size)
    {
        book_entry_t *cur = book->list + book->size;

        if (fread(&cur->key,       8, 1, f) != 1
         || fread(&cur->nodes,     8, 1, f) != 1
         || fread(&cur->score,     2, 1, f) != 1
         || fread(&cur->count,     2, 1, f) != 1
         || fread(&cur->inBook,    2, 1, f) != 1
         || fread(&cur->prevCount, 2, 1, f) != 1)
        {
            book_destroy(book);
            fclose(f);
            return (-1);
        }

        cur->key = book_get_u64(cur->key);
        cur->nodes = book_get_u64(cur->nodes);
        cur->score = (int16_t)book_get_u16((uint16_t)cur->score);
        cur->count = book_get_u16(cur->count);
        cur->inBook = book_get_u16(cur->inBook);
        cur->prevCount = book_get_u16(cur->prevCount);
        cur->list = malloc(cur->inBook * sizeof(book_next_t));

        if (cur->list == NULL && cur->inBook != 0)
        {
            book_destroy(book);
            fclose(f);
            return (-1);
        }

        cur->prevList = malloc(cur->prevCount * sizeof(hashkey_t));

        if (cur->prevList == NULL && cur->prevCount != 0)
        {
            free(cur->list);
            book_destroy(book);
            fclose(f);
            return (-1);
        }

        for (uint16_t i = 0; i < cur->inBook; ++i)
        {
            uint16_t move;
            uint64_t key;

            if (fread(&move, 2, 1, f) != 1 || fread(&key, 8, 1, f) != 1)
            {
                free(cur->list);
                free(cur->prevList);
                book_destroy(book);
                fclose(f);
                return (-1);
            }

            cur->list[i].move = (move_t)book_get_u16(move);
            cur->list[i].key = book_get_u64(key);
        }

        if (fread(cur->prevList, 8, cur->prevCount, f) != (size_t)cur->prevCount)
        {
            free(cur->list);
            free(cur->prevList);
            book_destroy(book);
            fclose(f);
            return (-1);
        }

        for (uint16_t i = 0; i < cur->prevCount; ++i)
            cur->prevList[i] = book_get_u64(cur->prevList[i]);
    }

    fclose(f);
    return (0);
}

int book_save(book_t *book, const char *filename)
{
    FILE *f = fopen(filename, "w");

    if (f == NULL)
        return (-1);

    {
        size_t tmp = book_get_z(book->capacity);

        if (fwrite(&tmp, sizeof(size_t), 1, f) != 1)
        {
            fclose(f);
            return (-1);
        }
    }

    for (size_t k = 0; k < book->capacity; ++k)
    {
        book_entry_t *cur = book->list + k;

        {
            book_entry_t tmp;

            tmp.key = book_get_u64(cur->key);
            tmp.nodes = book_get_u64(cur->nodes);
            tmp.score = (int16_t)book_get_u16((uint16_t)cur->score);
            tmp.count = book_get_u16(cur->count);
            tmp.inBook = book_get_u16(cur->inBook);
            tmp.prevCount = book_get_u16(cur->prevCount);

            if (fwrite(&tmp.key,       8, 1, f) != 1
             || fwrite(&tmp.nodes,     8, 1, f) != 1
             || fwrite(&tmp.score,     2, 1, f) != 1
             || fwrite(&tmp.count,     2, 1, f) != 1
             || fwrite(&tmp.inBook,    2, 1, f) != 1
             || fwrite(&tmp.prevCount, 2, 1, f) != 1)
            {
                fclose(f);
                return (-1);
            }
        }

        for (uint16_t i = 0; i < cur->inBook; ++i)
        {
            uint16_t move = (move_t)book_get_u16((uint16_t)cur->list[i].move);
            uint64_t key = book_get_u64(cur->list[i].key);

            if (fwrite(&move, 2, 1, f) != 1 || fwrite(&key, 8, 1, f) != 1)
            {
                fclose(f);
                return (-1);
            }
        }

        for (uint16_t i = 0; i < cur->prevCount; ++i)
        {
            hashkey_t key = book_get_u64(cur->prevList[i]);

            if (fwrite(&key, 8, 1, f) != 1)
            {
                fclose(f);
                return (-1);
            }
        }
    }

    fclose(f);
    return (0);
}

void book_info(book_t *book, const board_t *board)
{
    book_entry_t *entry = book_probe(book, board);

    if (entry == NULL)
    {
        printf("info string no entries found for this position\n");
        return ;
    }

    printf("info bookScore %s bookNodes %" FMT_INFO " bookMoves %u\n",
        score_to_str(entry->score), (info_t)entry->nodes, (unsigned int)entry->inBook);

    for (uint16_t i = 0; i < entry->inBook; ++i)
    {
        book_entry_t *nextEntry = book_probe_key(book, entry->list[i].key);
        printf("info bookMove %s bookMoveScore %s bookNodes %" FMT_INFO "\n",
            score_to_str(-nextEntry->score), move_to_str(entry->list[i].move, board->chess960), (info_t)nextEntry->nodes);
    }
    fflush(stdout);
}
