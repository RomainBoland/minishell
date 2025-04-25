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
	pids = malloc(sizeof(pid_t) * pipeline->cmd_count);
	heredoc_fds = malloc(sizeof(int) * pipeline->cmd_count);
	if (!pids || !heredoc_fds)
	{
		free(pids);
		free(heredoc_fds);
		return (1);
	}
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

/* Check if pipeline is a single builtin command */
int	is_single_builtin(t_pipeline *pipeline)
{
	return (pipeline->cmd_count == 1 && pipeline->commands[0]->args
		&& is_builtin(pipeline->commands[0]->args[0]));
}

/* Setup heredocs for all commands in pipeline */
int	setup_heredocs(t_pipeline *pipeline, t_shell *shell, int *heredoc_fds)
{
	int	i;

	for (i = 0; i < pipeline->cmd_count; i++)
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

/* Execute all commands in pipeline */
int	execute_pipeline_commands(t_pipeline *pipeline, t_shell *shell,
		pid_t *pids, int *heredoc_fds)
{
	int	i;
	int	in_fd;
	int	pipefds[2][2];
	int	active_pipe;

	i = 0;
	active_pipe = 0;
	if (pipeline->cmd_count > 1)
		if (pipe(pipefds[active_pipe]) < 0)
			return (handle_pipe_error(heredoc_fds, pipeline->cmd_count));
	while (i < pipeline->cmd_count)
	{
		in_fd = (i == 0) ? STDIN_FILENO : pipefds[1 - active_pipe][0];
		if (execute_pipeline_command(pipeline, shell, i, &active_pipe,
				pipefds, in_fd, pids, heredoc_fds) != 0)
			return (1);
		i++;
	}
	return (wait_for_pipeline(pids, pipeline->cmd_count));
}
