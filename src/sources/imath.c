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

#include "bitboard.h"
#include "imath.h"

uint64_t isqrt(uint64_t value)
{
    // Early handling of the zero case to avoid issues with bb_last_sq()
    // returning -1 (due to the operation '63 - lzcnt(0)').
    if (!value)
        return (0);

    uintmax_t root = 0;
    uintmax_t bit = 1 << (bb_last_sq(value) / 2);

    while (bit)
    {
        root >>= 1;
        if (value >= root + bit)
        {
            value -= root + bit;
            root += bit;
        }
        bit >>= 2;
    }

    return (root);
}