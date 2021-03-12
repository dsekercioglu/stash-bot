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

#ifndef MOVELIST_H
# define MOVELIST_H

# include <stdbool.h>
# include <stddef.h>
# include <stdint.h>
# include "board.h"
# include "inlining.h"
# include "move.h"

typedef struct  extmove_s
{
    move_t  move;
    score_t score;
}               extmove_t;

typedef struct  movelist_s
{
    extmove_t   moves[256];
    extmove_t   *last;
}               movelist_t;

extern movelist_t   SearchMoves;

extmove_t   *generate_all(extmove_t *movelist, const board_t *board);
extmove_t   *generate_classic(extmove_t *movelist, const board_t *board);
extmove_t   *generate_evasions(extmove_t *movelist, const board_t *board);
extmove_t   *generate_captures(extmove_t *movelist, const board_t *board);
extmove_t   *generate_quiet(extmove_t *movelist, const board_t *board);

extmove_t   *generate_piece_moves(extmove_t *movelist, const board_t *board,
            color_t us, piecetype_t pt, bitboard_t target_squares);

void        place_top_move(extmove_t *begin, extmove_t *end);

INLINED void    list_all(movelist_t *movelist, const board_t *board)
{
    movelist->last = generate_all(movelist->moves, board);
}

INLINED void    list_instable(movelist_t *movelist, const board_t *board)
{
    movelist->last = board->stack->checkers
        ? generate_evasions(movelist->moves, board)
        : generate_captures(movelist->moves, board);
}

INLINED void    list_pseudo(movelist_t *movelist, const board_t *board)
{
    movelist->last = board->stack->checkers
        ? generate_evasions(movelist->moves, board)
        : generate_classic(movelist->moves, board);
}

INLINED size_t  movelist_size(const movelist_t *movelist)
{
    return (movelist->last - movelist->moves);
}

INLINED const extmove_t *movelist_begin(const movelist_t *movelist)
{
    return (movelist->moves);
}

INLINED const extmove_t *movelist_end(const movelist_t *movelist)
{
    return (movelist->last);
}

INLINED bool    movelist_has_move(const movelist_t *movelist, move_t move)
{
    for (const extmove_t *extmove = movelist_begin(movelist);
            extmove < movelist_end(movelist); ++extmove)
        if (extmove->move == move)
            return (true);

    return (false);
}

INLINED extmove_t   *create_promotions(extmove_t *movelist, square_t to, direction_t direction)
{
    (movelist++)->move = create_promotion(to - direction, to, QUEEN);
    (movelist++)->move = create_promotion(to - direction, to, ROOK);
    (movelist++)->move = create_promotion(to - direction, to, BISHOP);
    (movelist++)->move = create_promotion(to - direction, to, KNIGHT);

    return (movelist);
}

#endif
