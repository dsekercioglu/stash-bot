/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci.c                                            .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2020/02/22 18:19:46 by stash        #+#   ##    ##    #+#       */
/*   Updated: 2020/02/24 13:33:50 by stash       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "uci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const cmdlink_t	commands[] =
{
	{"bench", &uci_bench},
	{"d", &uci_d},
	{"go", &uci_go},
	{"isready", &uci_isready},
	{"position", &uci_position},
	{"quit", &uci_quit},
	{"setoption", &uci_setoption},
	{"stop", &uci_stop},
	{"uci", &uci_uci},
	{"ucinewgame", &uci_ucinewgame},
	{NULL, NULL}
};

int		execute_uci_cmd(const char *command)
{
	char	*dup = strdup(command);

	char	*cmd = strtok(dup, " \t\n");

	if (!cmd)
	{
		free(dup);
		return (1);
	}

	for (size_t i = 0; commands[i].cmd_name != NULL; ++i)
	{
		if (strcmp(commands[i].cmd_name, cmd) == 0)
		{
			commands[i].call(strtok(NULL, ""));
			break ;
		}
	}

	if (strcmp(cmd, "quit") == 0)
		return (0);

	return (1);
}

void	uci_loop(int argc, char **argv)
{
	// Small hack to allow the engine thread to be ready before us

	usleep(1000);

	if (argc > 1)
		for (int i = 1; i < argc; ++i)
			execute_uci_cmd(argv[i]);
	else
	{
		char	*line = malloc(8192);

		while (fgets(line, 8192, stdin) != NULL)
			if (execute_uci_cmd(line) == 0)
				break ;

		free(line);
	}

	usleep(10000);
	pthread_mutex_lock(&g_engine_mutex);
	while (g_engine_mode != WAITING)
	{
		pthread_mutex_unlock(&g_engine_mutex);
		usleep(1000);
		pthread_mutex_lock(&g_engine_mutex);
	}
	pthread_mutex_unlock(&g_engine_mutex);
	uci_quit(NULL);
}
