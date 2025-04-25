/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_pipeline_process.c                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/25 09:49:25 by rboland           #+#    #+#             */
/*   Updated: 2025/04/25 09:49:25 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

/* Create process for a command in pipeline */
pid_t	create_command_process(t_pipeline *pipeline, t_shell *shell, int i,
		int in_fd, int out_fd, int *heredoc_fds)
{
	pid_t	pid;

	pid = fork();
	if (pid < 0)
	{
		perror("fork");
		return (-1);
	}
	if (pid == 0)
		execute_pipeline_child(pipeline, shell, i, in_fd, out_fd, heredoc_fds);
	return (pid);
}

/* Execute command in child process */
void	execute_pipeline_child(t_pipeline *pipeline, t_shell *shell, int i,
		int in_fd, int out_fd, int *heredoc_fds)
{
	int	j;

	setup_child_process_signals(pipeline->commands[i]);
	j = 0;
	while (j < pipeline->cmd_count)
	{
		if (j != i && heredoc_fds[j] != STDIN_FILENO)
			close(heredoc_fds[j]);
		j++;
	}
	free(heredoc_fds);
	handle_child_redirections(pipeline, i, in_fd, out_fd, heredoc_fds);
	execute_child_process(pipeline, shell, i);
}

/* Handle child process redirections */
void	handle_child_redirections(t_pipeline *pipeline, int i, int in_fd,
		int out_fd, int *heredoc_fds)
{
	int				input_redirected;

	input_redirected = handle_input_redirections(pipeline->commands[i]);
	if (!input_redirected)
		setup_command_input(i, in_fd, heredoc_fds);
	handle_output_redirections(pipeline->commands[i], out_fd);
}

/* Handle input redirections */
int	handle_input_redirections(t_command *cmd)
{
	t_redirection	*redir;
	int				fd;

	redir = cmd->redirections;
	while (redir)
	{
		if (redir->type == TOKEN_REDIR_IN)
		{
			fd = open(redir->file, O_RDONLY);
			if (fd < 0)
			{
				perror(redir->file);
				exit(1);
			}
			dup2(fd, STDIN_FILENO);
			close(fd);
			return (1);
		}
		redir = redir->next;
	}
	return (0);
}

/* Setup command input */
void	setup_command_input(int i, int in_fd, int *heredoc_fds)
{
	if (i > 0)
	{
		dup2(in_fd, STDIN_FILENO);
		close(in_fd);
	}
	else if (heredoc_fds[i] != STDIN_FILENO)
	{
		dup2(heredoc_fds[i], STDIN_FILENO);
		close(heredoc_fds[i]);
	}
}
