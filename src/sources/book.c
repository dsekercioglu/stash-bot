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

#include <endian.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "book.h"
#include "imath.h"
#include "uci.h"

book_t Book = {};

bool book_read_u64(FILE *f, uint64_t *var)
{
    if (fread(var, 8, 1, f) == 0)
        return (false);

#if (BYTE_ORDER == BIG_ENDIAN)
    uint64_t q1 = UINT64_C(0x00FF00FF00FF00FF);
    uint64_t q2 = UINT64_C(0x0000FFFF0000FFFF);

    *var = ((*var <<  8) & ~q1) | ((*var >>  8) & q1);
    *var = ((*var << 16) & ~q2) | ((*var >> 16) & q2);
    *var =  (*var << 32)        |  (*var >> 32);
#endif

    return (true);
}

bool book_read_u32(FILE *f, uint32_t *var)
{
    if (fread(var, 4, 1, f) == 0)
        return (false);

#if (BYTE_ORDER == BIG_ENDIAN)
    uint64_t d1 = UINT32_C(0x00FF00FF);

    *var = ((*var <<  8) & ~d1) | ((*var >>  8) & d1);
    *var =  (*var << 16)        |  (*var >> 16);
#endif

    return (true);
}

bool book_read_u16(FILE *f, uint16_t *var)
{
    if (fread(var, 2, 1, f) == 0)
        return (false);

#if (BYTE_ORDER == BIG_ENDIAN)
    *var = (*var << 8) | (*var >> 8);
#endif

    return (true);
}

bool book_read_u8(FILE *f, uint8_t *var)
{
    return (fread(var, 1, 1, f) == 1);
}

bool book_write_u64(FILE *f, uint64_t var)
{

#if (BYTE_ORDER == BIG_ENDIAN)
    uint64_t q1 = UINT64_C(0x00FF00FF00FF00FF);
    uint64_t q2 = UINT64_C(0x0000FFFF0000FFFF);

    var = ((var <<  8) & ~q1) | ((var >>  8) & q1);
    var = ((var << 16) & ~q2) | ((var >> 16) & q2);
    var =  (var << 32)        |  (var >> 32);
#endif

    return (fwrite(&var, 8, 1, f) == 1);
}

bool book_write_u32(FILE *f, uint32_t var)
{

#if (BYTE_ORDER == BIG_ENDIAN)
    uint64_t d1 = UINT32_C(0x00FF00FF);

    var = ((var <<  8) & ~d1) | ((var >>  8) & d1);
    var =  (var << 16)        |  (var >> 16);
#endif

    return (fwrite(&var, 4, 1, f) == 1);
}

bool book_write_u16(FILE *f, uint16_t var)
{
#if (BYTE_ORDER == BIG_ENDIAN)
    var = (var << 8) | (var >> 8);
#endif

    return (fwrite(&var, 2, 1, f) == 1);
}

bool book_write_u8(FILE *f, uint8_t var)
{
    return (fwrite(&var, 1, 1, f) == 1);
}

double score_to_winrate(score_t score)
{
    if (score >= VICTORY)
        return (1.0);
    else if (score <= -VICTORY)
        return (0.0);
    else
        return (1.0 / (1.0 + exp(-score / 200.0)));
}

book_entry_t *book_get(book_t *book, hashkey_t key, bool strict)
{
    if (book->maxSize == 0)
        return (NULL);

    size_t left = 0;
    size_t right = book->size;

    while (left < right)
    {
        size_t mid = (left + right) / 2;
        book_entry_t *cur = book->entries + mid;

        if (cur->key < key)
            right = mid;
        else if (cur->key > key)
            left = mid + 1;
        else
            return (cur);
    }

    book_entry_t *cur = book->entries + left;

    return ((!strict || cur->key == key) ? cur : NULL);
}

const book_entry_t *book_const_get(const book_t *book, hashkey_t key, bool strict)
{
    if (book->maxSize == 0)
        return (NULL);

    size_t left = 0;
    size_t right = book->size;

    while (left < right)
    {
        size_t mid = (left + right) / 2;
        const book_entry_t *cur = book->entries + mid;

        if (cur->key < key)
            right = mid;
        else if (cur->key > key)
            left = mid + 1;
        else
            return (cur);
    }

    const book_entry_t *cur = book->entries + left;

    return ((!strict || cur->key == key) ? cur : NULL);
}

void book_free(book_t *book)
{
    for (size_t i = 0; i < book->size; ++i)
    {
        free(book->entries[i].prevKeys);
        free(book->entries[i].nextMoves);
    }
    free(book->entries);
    book->size = 0;
    book->maxSize = 0;
    book->entries = NULL;
}

int book_load(book_t *book, const char *filename)
{
    srand48(time(NULL));
    if (book->size)
        book_free(book);

    FILE *f = fopen(filename, "r");

    if (f == NULL)
        return (-1);

    uint64_t bookSize;

    if (!book_read_u64(f, &bookSize))
        goto book_load_error;

    book->entries = malloc(sizeof(book_entry_t) * bookSize);
    if (book->entries == NULL)
        goto book_load_error;
    book->maxSize = bookSize;

    for (book->size = 0; book->size < bookSize; ++book->size)
    {
        book_entry_t *cur = book->entries + book->size;

        if (!book_read_u64(f, &cur->key)
         || !book_read_u32(f, &cur->kNodes)
         || !book_read_u16(f, (uint16_t *)&cur->score)
         || !book_read_u8 (f, &cur->prevCount)
         || !book_read_u8 (f, &cur->nextCount))
            goto book_load_error;

        cur->prevKeys = malloc(sizeof(hashkey_t) * cur->prevCount);

        if (cur->prevKeys == NULL && cur->prevCount != 0)
            goto book_load_error;

        cur->nextKeys = malloc(sizeof(hashkey_t) * cur->nextCount);

        if (cur->nextKeys == NULL && cur->nextCount != 0)
        {
            free(cur->prevKeys);
            goto book_load_error;
        }

        cur->nextMoves = malloc(sizeof(move_t) * cur->nextCount);

        if (cur->nextMoves == NULL && cur->nextCount != 0)
        {
            free(cur->prevKeys);
            free(cur->nextKeys);
            goto book_load_error;
        }

        for (uint8_t i = 0; i < cur->prevCount; ++i)
            if (!book_read_u64(f, cur->prevKeys + i))
            {
                free(cur->prevKeys);
                free(cur->nextKeys);
                free(cur->nextMoves);
                goto book_load_error;
            }

        for (uint8_t i = 0; i < cur->nextCount; ++i)
            if (!book_read_u64(f, cur->nextKeys + i))
            {
                free(cur->prevKeys);
                free(cur->nextKeys);
                free(cur->nextMoves);
                goto book_load_error;
            }

        uint16_t move;

        for (uint8_t i = 0; i < cur->nextCount; ++i)
        {
            if (!book_read_u16(f, &move))
            {
                free(cur->prevKeys);
                free(cur->nextKeys);
                free(cur->nextMoves);
                goto book_load_error;
            }
            cur->nextMoves[i] = (move_t)(uint32_t)move;
        }
    }

    fclose(f);
    return (0);

book_load_error:
    fclose(f);
    book_free(book);
    return (-2);
}

int book_save(const book_t *book, const char *filename)
{
    FILE *f = fopen(filename, "w");

    if (f == NULL)
        return (-1);

    if (!book_write_u64(f, book->size))
        goto book_save_error;

    for (size_t k = 0; k < book->size; ++k)
    {
        const book_entry_t *cur = book->entries + k;

        if (!book_write_u64(f, cur->key)
         || !book_write_u32(f, cur->kNodes)
         || !book_write_u16(f, cur->score)
         || !book_write_u8 (f, cur->prevCount)
         || !book_write_u8 (f, cur->nextCount))
            goto book_save_error;

        for (uint8_t i = 0; i < cur->prevCount; ++i)
            if (!book_write_u64(f, cur->prevKeys[i]))
                goto book_save_error;

        for (uint8_t i = 0; i < cur->nextCount; ++i)
            if (!book_write_u64(f, cur->nextKeys[i]))
                goto book_save_error;

        for (uint8_t i = 0; i < cur->nextCount; ++i)
            if (!book_write_u16(f, (uint16_t)(uint32_t)cur->nextMoves[i]))
                goto book_save_error;
    }

    fclose(f);
    return (0);

book_save_error:
    fclose(f);
    return (-2);
}

void book_probe(const book_t *book, hashkey_t key, score_t *score, move_t *move, uint32_t *kNodes, double R)
{
    *score = NO_SCORE;
    *move = NO_MOVE;
    const book_entry_t *entry = book_const_get(book, key, true);

    if (entry == NULL || entry->nextCount == 0)
        return ;

    double totalW = 0.0;

    for (uint8_t i = 0; i < entry->nextCount; ++i)
    {
        const book_entry_t *next = book_const_get(book, entry->nextKeys[i], true);
        if (next == NULL)
            return ;
        double winrate = score_to_winrate(-next->score);
        totalW += (R == 0.0) ? 1.0 : pow(winrate, R);
    }

    totalW *= drand48();
    uint8_t i;

    for (i = entry->nextCount - 1; i > 0; --i)
    {
        const book_entry_t *next = book_const_get(book, entry->nextKeys[i], true);
        double winrate = score_to_winrate(-next->score);
        totalW -= (R == 0.0) ? 1.0 : pow(winrate, R);
        if (totalW <= 0.0)
            break ;
    }

    const book_entry_t *next = book_const_get(book, entry->nextKeys[i], true);

    *move = entry->nextMoves[i];
    *score = -next->score;
    *kNodes = next->kNodes;
}

book_entry_t *book_create_entry(book_t *book, hashkey_t key)
{
    if (book->size == book->maxSize)
    {
        // Quadratic growth: since the book size raises rather slowly, we go
        // for small capacity increases rather than exponential ones.
        size_t newMaxSize = book->maxSize + isqrt(book->maxSize) + 1;
        void *newPtr = realloc(book->entries, sizeof(book_entry_t) * newMaxSize);

        if (newPtr == NULL)
            return (NULL);
        book->entries = newPtr;
        book->maxSize = newMaxSize;
    }

    book_entry_t *where = book_get(book, key, false);

    memmove(where + 1, where, (uintptr_t)(book->entries + book->size) - (uintptr_t)where);
    *where = (book_entry_t){key, 0, NO_SCORE, 0, 0, NULL, NULL, NULL};
    book->size++;

    return (where);
}

void book_backpropagate(book_t *book, book_entry_t *entry)
{
    for (uint8_t i = 0; i < entry->prevCount; ++i)
    {
        book_entry_t *prev = book_get(book, entry->prevKeys[i], true);

        if (entry->kNodes >= prev->kNodes / 4 && prev->score > -entry->score)
        {
            prev->score = -entry->score;
            book_backpropagate(book, prev);
        }
    }
}

bool book_entry_better_than(book_t *book, hashkey_t left, hashkey_t right)
{
    book_entry_t *lEntry = book_get(book, left, true);
    book_entry_t *rEntry = book_get(book, right, true);

    return (rEntry->score > lEntry->score);
}

void book_sort_entry_moves(book_t *book, book_entry_t *entry)
{
    const int size = (int)entry->nextCount;

    for (int i = 1; i < size; ++i)
    {
        hashkey_t tmpKey = entry->nextKeys[i];
        move_t tmpMove = entry->nextMoves[i];
        int j = i - 1;

        while (j >= 0 && book_entry_better_than(book, tmpKey, entry->nextKeys[j]))
        {
            entry->nextKeys[j + 1] = entry->nextKeys[j];
            entry->nextMoves[j + 1] = entry->nextMoves[j];
            --j;
        }
        entry->nextKeys[j + 1] = tmpKey;
        entry->nextMoves[j + 1] = tmpMove;
    }
}

int book_update_entry(book_t *book, hashkey_t key, hashkey_t nextKey,
    uint32_t kNodes, uint32_t nextKNodes, move_t bestmove, score_t score)
{
    book_entry_t *ourEntry = book_get(book, key, true);

    if (ourEntry == NULL)
    {
        ourEntry = book_create_entry(book, key);
        if (ourEntry == NULL)
            return (-1);
    }

    if (ourEntry->score == NO_SCORE || ourEntry->kNodes < kNodes)
    {
        ourEntry->kNodes = kNodes;
        ourEntry->score = score;

        book_backpropagate(book, ourEntry);
    }

    bool nextFound = false;

    for (uint8_t i = 0; i < ourEntry->nextCount; ++i)
    {
        if (ourEntry->nextMoves[i] == bestmove)
        {
            nextFound = true;
            book_entry_t *theirEntry = book_get(book, nextKey, true);

            if (theirEntry->kNodes < nextKNodes)
            {
                theirEntry->kNodes = nextKNodes;
                theirEntry->score = -score;
                book_sort_entry_moves(book, ourEntry);
            }
        }
    }

    if (!nextFound)
    {
        void *ptr = realloc(ourEntry->nextMoves, sizeof(move_t) * (ourEntry->nextCount + 1));
        if (ptr == NULL)
            return (-1);
        ourEntry->nextMoves = ptr;

        ptr = realloc(ourEntry->nextKeys, sizeof(hashkey_t) * (ourEntry->nextCount + 1));
        if (ptr == NULL)
            return (-1);
        ourEntry->nextKeys = ptr;

        ourEntry->nextMoves[ourEntry->nextCount] = bestmove;
        ourEntry->nextKeys[ourEntry->nextCount] = nextKey;

        book_entry_t *theirEntry = book_get(book, nextKey, true);

        if (theirEntry == NULL)
        {
            theirEntry = book_create_entry(book, nextKey);
            if (theirEntry == NULL)
                return (-1);
            ourEntry = book_get(book, key, true);
        }

        ptr = realloc(theirEntry->prevKeys, sizeof(hashkey_t) * (theirEntry->prevCount + 1));
        if (ptr == NULL)
            return (-1);
        theirEntry->prevKeys = ptr;

        theirEntry->prevKeys[theirEntry->prevCount] = key;

        ourEntry->nextCount++;
        theirEntry->prevCount++;

        if (theirEntry->kNodes < nextKNodes)
        {
            theirEntry->kNodes = nextKNodes;
            theirEntry->score = -score;
            book_sort_entry_moves(book, ourEntry);
        }
    }

    return (0);
}

void book_tree(const book_t *book, hashkey_t key, int depth)
{
    const book_entry_t *entry = book_const_get(book, key, true);

    if (entry == NULL)
        return ;

    if (depth == 0)
        printf("\n");

    char unit;
    double value;
    int precision;

    if (entry->kNodes < 1000)
    {
        unit = 'k';
        value = entry->kNodes;
        precision = 0;
    }
    else if (entry->kNodes < 1000000)
    {
        unit = 'M';
        value = entry->kNodes / 1000.0;
        precision = 2 - (entry->kNodes >= 10000) - (entry->kNodes >= 100000);
    }
    else
    {
        unit = 'G';
        value = entry->kNodes / 1000000.0;
        precision = 2 - (entry->kNodes >= 10000000) - (entry->kNodes >= 100000000);
    }

    printf("%*sEval %+.2lf (%.*lf%c nodes)\n", depth, "", entry->score / 100.0, precision, value, unit);

    for (uint8_t i = 0; i < entry->nextCount; ++i)
    {
        if (i > 0)
            printf("\n");
        printf("%*sAfter move %s:\n", depth, "", move_to_str(entry->nextMoves[i], false));
        book_tree(book, entry->nextKeys[i], depth + 4);
    }

    if (depth == 0)
    {
        printf("\n");
        fflush(stdout);
    }
}