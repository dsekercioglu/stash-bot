#include "imath.h"
#include "uci.h"
#include "engine.h"

// Next values are derived from the formula:
// score = 800 * log10(RmobPoints / (1 - RmobPoints)).
// e.g. stalemate gives 0.75 points at TCEC,
// so score = 800 * log10(0.75 / 0.25) ~= 382.

const score_t	RmobScores[31] = {
	0, 0, 0, 0, 0,
	-1, -1, -3, -5, -11,
	-22, -43, -87, -177, -382,

	0, // Used for G=0, i.e. draw

	382, 177, 87, 43, 22,
	11, 5, 3, 1, 1,
	0, 0, 0, 0, 0
};

score_t	eval_rmob(const board_t *board, int plies)
{
	extern ucioptions_t		g_options;

	// Use draw score for standard chess
	if (g_options.variant == Chess)
		return (0);

	// Get r-mobility score

	int		w_rmob = board->stack->rmobility[WHITE];
	int		b_rmob = board->stack->rmobility[BLACK];

	int		G = (w_rmob > b_rmob) ? b_rmob
			: (b_rmob > w_rmob) ? -w_rmob : 0;

	// Transform r-mobility to a score for r-mobility chess
	if (g_options.variant == Rmob)
	{
		score_t		rmob_score = RmobScores[clamp(G, -15, 15) + 15];

		return (board->side_to_move == WHITE ? rmob_score : -rmob_score);
	}

	// In r-mobility Armaggeddon, return a large positive score if we're winning
	// our bet, a large negative score if we're losing it, or draw if we're doing
	// exactly our bet

	G = G * 2 + (G > 0) - (G < 0);

	score_t		komi_score;

	if (g_options.komi > 0)
		komi_score = (G < 0 || G > g_options.komi) ? mated_in(plies)
			: (G == g_options.komi) ? 0 : mate_in(plies);
	else
		komi_score = (G > 0 || G < g_options.komi) ? mate_in(plies)
			: (G == g_options.komi) ? 0 : mated_in(plies);

	return (board->side_to_move == WHITE ? komi_score : -komi_score);
}
