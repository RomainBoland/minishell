/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_pipeline_cmd.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/25 13:59:31 by rboland           #+#    #+#             */
/*   Updated: 2025/04/25 13:59:31 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

/* Initialize pipe for pipeline execution */
int	init_pipeline_pipe(t_pipeline *pipeline, int pipefds[2][2])
{
	if (pipeline->cmd_count > 1)
	{
		if (pipe(pipefds[0]) < 0)
			return (0);
	}
	return (1);
}

/* Setup pipe for next command */
int	setup_next_pipe(t_pipeline *pipeline, int i, int pipefds[2][2], 
					int active_pipe)
{
	if (i < pipeline->cmd_count - 2)
	{
		if (pipe(pipefds[1 - active_pipe]) < 0)
			return (0);
	}
	return (1);
}

/* Execute pipeline commands loop */
int	execute_pipeline_commands(t_pipeline *pipeline, t_shell *shell,
		pid_t *pids, int *heredoc_fds)
{
	int	i;
	int	in_fd;
	int	pipefds[2][2];
	int	active_pipe;

	i = 0;
	active_pipe = 0;
	if (!init_pipeline_pipe(pipeline, pipefds))
		return (handle_pipe_error(heredoc_fds, pipeline->cmd_count));
	while (i < pipeline->cmd_count)
	{
		in_fd = (i == 0) ? STDIN_FILENO : pipefds[1 - active_pipe][0];
		if (!execute_pipeline_iter(pipeline, shell, i, &active_pipe,
				pipefds, in_fd, pids, heredoc_fds))
			return (1);
		i++;
	}
	return (wait_for_pipeline(pids, pipeline->cmd_count));
}

/* Single iteration of pipeline execution */
int	execute_pipeline_iter(t_pipeline *pipeline, t_shell *shell, int i, 
		int *active_pipe, int pipefds[2][2], int in_fd, 
		pid_t *pids, int *heredoc_fds)
{
	int	out_fd;
	int	res;

	if (i == pipeline->cmd_count - 1)
		out_fd = STDOUT_FILENO;
	else
	{
		out_fd = pipefds[*active_pipe][1];
		if (!setup_next_pipe(pipeline, i, pipefds, *active_pipe))
			return (handle_pipe_iter_error(pipeline, i, in_fd, pipefds, 
				*active_pipe, heredoc_fds, pids));
	}
	res = execute_pipeline_cmd(pipeline, shell, i, in_fd, out_fd, 
		*active_pipe, pipefds, pids, heredoc_fds);
	if (!res)
		return (0);
	*active_pipe = 1 - *active_pipe;
	return (1);
}

/* Execute a single command in the pipeline */
int	execute_pipeline_cmd(t_pipeline *pipeline, t_shell *shell, int i, 
		int in_fd, int out_fd, int active_pipe, int pipefds[2][2], 
		pid_t *pids, int *heredoc_fds)
{
	pids[i] = fork();
	if (pids[i] < 0)
	{
		perror("fork");
		return (0);
	}
	if (pids[i] == 0)
	{
		close_unused_pipes(pipeline, i, active_pipe, pipefds);
		setup_pipe_redirects(in_fd, out_fd);
		execute_pipeline_command_child(pipeline, shell, i, heredoc_fds);
	}
	handle_parent_pipes_fixed(i, in_fd, active_pipe, pipefds, pipeline);
	return (1);
}
