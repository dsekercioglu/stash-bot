/*
**    Stash, a UCI chess playing engine developed from scratch
**    Copyright (C) 2019-2022 Morgan Houppin
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

#ifndef IMATH_H
# define IMATH_H

# include "inlining.h"

INLINED int max(int a, int b)
{
    return (a > b ? a : b);
}

INLINED int min(int a, int b)
{
    return (a < b ? a : b);
}

INLINED int clamp(int value, int lower, int upper)
{
    return (value < lower ? lower : value > upper ? upper : value);
}

#endif
