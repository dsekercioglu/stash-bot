/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   uci_isready.c                                    .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: mhouppin <mhouppin@student.le-101.>        +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/10/28 15:25:12 by mhouppin     #+#   ##    ##    #+#       */
/*   Updated: 2019/10/31 06:25:47 by mhouppin    ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include <stdio.h>
#include <unistd.h>
#include "engine.h"

void	uci_isready(const char *args)
{
	(void)args;

	pthread_mutex_lock(&mtx_engine);
	while (g_engine_mode == THINKING)
	{
		pthread_mutex_unlock(&mtx_engine);
		usleep(60);
		pthread_mutex_lock(&mtx_engine);
	}

	pthread_mutex_unlock(&mtx_engine);
	puts("readyok");
}
