/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_pipeline.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/25 09:48:26 by rboland           #+#    #+#             */
/*   Updated: 2025/04/25 09:48:26 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

/* Prepare pipeline resources */
int	prepare_pipeline_resources(t_pipeline *pipeline, pid_t **pids,
							int **heredoc_fds)
{
	*pids = malloc(sizeof(pid_t) * pipeline->cmd_count);
	*heredoc_fds = malloc(sizeof(int) * pipeline->cmd_count);
	if (!*pids || !*heredoc_fds)
	{
		free(*pids);
		free(*heredoc_fds);
		return (0);
	}
	return (1);
}

/* Check if pipeline is a single builtin command */
int	is_single_builtin(t_pipeline *pipeline)
{
	return (pipeline->cmd_count == 1 && pipeline->commands[0]->args
		&& is_builtin(pipeline->commands[0]->args[0]));
}

/* Execute a pipeline of commands */
int	execute_pipeline(t_pipeline *pipeline, t_shell *shell)
{
	int		exit_status;
	pid_t	*pids;
	int		*heredoc_fds;

	if (!pipeline || pipeline->cmd_count == 0)
		return (1);
	if (is_single_builtin(pipeline))
		return (execute_command(pipeline->commands[0],
				STDIN_FILENO, STDOUT_FILENO, shell));
	if (!prepare_pipeline_resources(pipeline, &pids, &heredoc_fds))
		return (1);
	if (!setup_heredocs(pipeline, shell, heredoc_fds))
	{
		free(pids);
		free(heredoc_fds);
		return (1);
	}
	exit_status = execute_pipeline_commands(pipeline, shell, pids, heredoc_fds);
	free(pids);
	free(heredoc_fds);
	return (exit_status);
}

/* Setup heredocs for all commands in pipeline */
int	setup_heredocs(t_pipeline *pipeline, t_shell *shell, int *heredoc_fds)
{
	int	i;

	i = 0;
	while (i < pipeline->cmd_count)
	{
		if (pipeline->commands[i]->has_heredoc)
		{
			heredoc_fds[i] = setup_heredoc(pipeline->commands[i], shell);
			if (heredoc_fds[i] < 0)
			{
				cleanup_heredocs(heredoc_fds, i);
				return (0);
			}
		}
		else
			heredoc_fds[i] = STDIN_FILENO;
		i++;
	}
	return (1);
}

/* Cleanup heredocs on error */
void	cleanup_heredocs(int *heredoc_fds, int count)
{
	int	i;

	i = 0;
	while (i < count)
	{
		if (heredoc_fds[i] != STDIN_FILENO)
			close(heredoc_fds[i]);
		i++;
	}
}
