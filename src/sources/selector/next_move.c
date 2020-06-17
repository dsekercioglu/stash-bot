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

#include "history.h"
#include "selector.h"

void	score_instable(selector_t *sel)
{
	for (extmove_t *ext = sel->cur; ext < sel->end; ++ext)
	{
		if (ext->move == sel->tt_move)
		{
			sel->end--;
			*ext = *sel->end;
			ext--;
			continue ;
		}

		switch (type_of_move(ext->move))
		{
			case PROMOTION:
				ext->score = 128;
				break ;

			case EN_PASSANT:
				ext->score = 64 + PAWN * 8 - PAWN;
				break ;

			default:
				ext->score = see_greater_than(sel->board, ext->move, -30) ? 64
					: 0;
				ext->score += type_of_piece(piece_on(sel->board,
					move_to_square(ext->move))) * 8;
				ext->score -= type_of_piece(piece_on(sel->board,
					move_from_square(ext->move)));
				break ;
		}
	}
}

void	score_quiet(selector_t *sel)
{
	const bool	in_endgame = popcount(sel->board->piecetype_bits[ALL_PIECES]) <= 16;
	const bool	black_to_move = sel->board->side_to_move == BLACK;

	for (extmove_t *ext = sel->cur; ext < sel->end; ++ext)
	{
		if (ext->move == sel->tt_move || ext->move == sel->killers[0]
			|| ext->move == sel->killers[1])
		{
			sel->end--;
			*ext = *sel->end;
			ext--;
			continue ;
		}

		const square_t		from = move_from_square(ext->move);
		const square_t		to = move_to_square(ext->move);
		const piece_t		piece = piece_on(sel->board, from);
		const scorepair_t	qscore = PsqScore[piece][to] - PsqScore[piece][from];

		ext->score = (in_endgame) ? endgame_score(qscore) : midgame_score(qscore);

		if (black_to_move)
			ext->score = -ext->score;

		ext->score += get_hist_score(piece, ext->move);
	}
}

void	score_evasions(selector_t *sel)
{
	const bool	in_endgame = popcount(sel->board->piecetype_bits[ALL_PIECES]) <= 16;
	const bool	black_to_move = sel->board->side_to_move == BLACK;

	for (extmove_t *ext = sel->cur; ext < sel->end; ++ext)
	{
		if (ext->move == sel->tt_move)
		{
			sel->end--;
			*ext = *sel->end;
			ext--;
			continue ;
		}

		const square_t		from = move_from_square(ext->move);
		const square_t		to = move_to_square(ext->move);
		const piece_t		piece = piece_on(sel->board, from);

		if (!empty_square(sel->board, from))
			ext->score = 2048 - type_of_piece(piece);
		else if (type_of_piece(piece) == KING)
		{
			const scorepair_t	qscore = PsqScore[piece][to] - PsqScore[piece][from];

			ext->score = (in_endgame) ? endgame_score(qscore) : midgame_score(qscore);

			if (black_to_move)
				ext->score = -ext->score;
		}
		else
			ext->score = -2048 - type_of_piece(piece);
	}
}

move_t	next_move(selector_t *sel)
{
__top:
	switch(sel->stage)
	{
		case MainTT:
		case EvasionTT:
			sel->stage++;
			if (sel->tt_move && board_tt_legal(sel->board, sel->tt_move))
				return (sel->tt_move);
			goto __top;

		case CaptureInit:
		case QcaptureInit:
			sel->stage++;
			sel->end = generate_captures(sel->end, sel->board);
			score_instable(sel);
			goto __top;

		case GoodCapture:
			if (sel->cur == sel->end)
			{
				sel->stage++;
				goto __top;
			}
			place_top_move(sel->cur, sel->end);
			if (sel->cur->score < 64)
			{
				sel->stage++;
				goto __top;
			}
			return ((sel->cur++)->move);

		case Killer:
			if (sel->kcount == 2)
			{
				sel->stage++;
				goto __top;
			}
			move_t	move = sel->killers[sel->kcount++];
			if (!move || !board_tt_legal(sel->board, move))
				goto __top;
			return (move);

		case BadCapture:
			if (sel->cur == sel->end)
			{
				sel->stage++;
				goto __top;
			}
			place_top_move(sel->cur, sel->end);
			return ((sel->cur++)->move);

		case InitQuiet:
			sel->stage++;
			sel->end = generate_quiet(sel->end, sel->board);
			score_quiet(sel);

		// Fallthrough
		case Quiet:
		case Evasion:
		case Qcapture:
			if (sel->cur == sel->end)
				return (NO_MOVE);
			place_top_move(sel->cur, sel->end);
			return ((sel->cur++)->move);

		case QsearchTT:
			sel->stage++;
			if (sel->tt_move
				&& is_capture_or_promotion(sel->board, sel->tt_move)
				&& board_tt_legal(sel->board, sel->tt_move))
				return (sel->tt_move);
			goto __top;

		case EvasionInit:
			sel->stage++;
			sel->end = generate_evasions(sel->end, sel->board);
			score_evasions(sel);
			goto __top;
	}
	__builtin_unreachable();
	return (NO_MOVE);
}
