/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_pipeline_utils.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/25 09:49:04 by rboland           #+#    #+#             */
/*   Updated: 2025/04/25 09:49:04 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

/* Handle pipe creation error */
int	handle_pipe_error(int *heredoc_fds, int cmd_count)
{
	perror("pipe");
	cleanup_heredocs(heredoc_fds, cmd_count);
	return (1);
}

/* Handle error in pipe iteration */
int	handle_pipe_iter_error(t_pipeline *pipeline, int i, int in_fd,
		int pipefds[2][2], int active_pipe, int *heredoc_fds, pid_t *pids)
{
	handle_pipe_error(heredoc_fds, pipeline->cmd_count);
	if (i > 0)
		close(in_fd);
	close(pipefds[active_pipe][0]);
	close(pipefds[active_pipe][1]);
	if (i < pipeline->cmd_count - 2)
	{
		close(pipefds[1 - active_pipe][0]);
		close(pipefds[1 - active_pipe][1]);
	}
	wait_for_processes(pids, i);
	return (0);
}

/* Close unused pipes in child process */
void	close_unused_pipes(t_pipeline *pipeline, int i, int active_pipe, 
		int pipefds[2][2])
{
	if (i < pipeline->cmd_count - 1)
		close(pipefds[active_pipe][0]);
	if (i < pipeline->cmd_count - 2)
	{
		close(pipefds[1 - active_pipe][0]);
		close(pipefds[1 - active_pipe][1]);
	}
	if (i > 0)
		close(pipefds[1 - active_pipe][1]);
}

/* Setup input/output redirections for pipeline command */
void	setup_pipe_redirects(int in_fd, int out_fd)
{
	if (in_fd != STDIN_FILENO)
	{
		dup2(in_fd, STDIN_FILENO);
		close(in_fd);
	}
	if (out_fd != STDOUT_FILENO)
	{
		dup2(out_fd, STDOUT_FILENO);
		close(out_fd);
	}
}

/* Handle parent process pipes with improved logic */
void	handle_parent_pipes_fixed(int i, int in_fd, int active_pipe,
		int pipefds[2][2], t_pipeline *pipeline)
{
	if (i > 0)
		close(in_fd);
	if (i < pipeline->cmd_count - 1)
		close(pipefds[active_pipe][1]);
}
