/*
**	Stash, a UCI chess playing engine developed from scratch
**	Copyright (C) 2019-2020 Morgan Houppin
**
**	Stash is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	Stash is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "board.h"
#include "movelist.h"

bool	board_tt_legal(const board_t *board, move_t move)
{
	color_t		us = board->side_to_move;
	square_t	from = move_from_square(move);
	square_t	to = move_to_square(move);
	piece_t		piece = piece_on(board, from);

	if (type_of_move(move) != NORMAL_MOVE)
	{
		movelist_t	list;

		list_all(&list, board);

		for (const extmove_t *ext = movelist_begin(&list);
			ext < movelist_end(&list); ++ext)
			if (ext->move == move)
				return (true);
		return (false);
	}

	if (promotion_type(move) != KNIGHT)
		return (false);

	if (piece == NO_PIECE || color_of_piece(piece) != us)
		return (false);

	bitboard_t	to_bit = square_bit(to);

	if (board->color_bits[us] & to_bit)
		return (false);

	if (type_of_piece(piece) == PAWN)
	{
		if ((RANK_1_BITS | RANK_8_BITS) & to_bit)
			return (false);

		if (!(PawnMoves[us][from] & board->color_bits[opposite_color(us)] & to_bit)
			&& !((from + pawn_direction(us) == to) && empty_square(board, to))
			&& !((from + 2 * pawn_direction(us) == to)
			&& (relative_square_rank(from, us) == RANK_2)
			&& empty_square(board, to)
			&& empty_square(board, to - pawn_direction(us))))
			return (false);
	}
	else if (!(piece_moves(type_of_piece(piece), from,
		board->piecetype_bits[ALL_PIECES]) & to_bit))
		return (false);

	if (board->stack->checkers)
	{
		if (type_of_piece(piece) != KING)
		{
			if (more_than_one(board->stack->checkers))
				return (false);

			if (!((squares_between(first_square(board->stack->checkers),
				board->piece_list[create_piece(us, KING)][0])
				| board->stack->checkers) & to_bit))
				return (false);
		}
		else if (attackers_list(board, to, board->piecetype_bits[ALL_PIECES]
			^ square_bit(from)) & board->color_bits[opposite_color(us)])
			return (false);
	}

	return (true);
}
