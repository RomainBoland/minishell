/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_process.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/25 09:47:44 by rboland           #+#    #+#             */
/*   Updated: 2025/04/25 09:47:44 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

/* Handle fork error */
int	handle_fork_error(char *exec_path, int heredoc_fd)
{
	perror("fork");
	free(exec_path);
	if (heredoc_fd != STDIN_FILENO)
		close(heredoc_fd);
	return (1);
}

/* Process redirections for command */
int	setup_cmd_redirections(t_command *cmd)
{
	t_redirection	*redir;
	int				fd;

	redir = cmd->redirections;
	while (redir)
	{
		if (redir->type == TOKEN_REDIR_IN)
			fd = setup_input_redir(redir);
		else if (redir->type == TOKEN_REDIR_OUT)
			fd = setup_output_redir(redir, O_TRUNC);
		else if (redir->type == TOKEN_APPEND)
			fd = setup_output_redir(redir, O_APPEND);
		if (fd < 0)
			return (0);
		redir = redir->next;
	}
	return (1);
}

/* Setup input redirection */
int	setup_input_redir(t_redirection *redir)
{
	int	fd;

	fd = open(redir->file, O_RDONLY);
	if (fd < 0)
	{
		perror(redir->file);
		return (-1);
	}
	dup2(fd, STDIN_FILENO);
	close(fd);
	return (fd);
}

/* Setup output redirection */
int	setup_output_redir(t_redirection *redir, int mode)
{
	int	fd;

	fd = open(redir->file, O_WRONLY | O_CREAT | mode, 0644);
	if (fd < 0)
	{
		perror(redir->file);
		return (-1);
	}
	dup2(fd, STDOUT_FILENO);
	close(fd);
	return (fd);
}

/* Child process execution */
void	child_process(t_command *cmd, int in_fd, int out_fd,
			char *exec_path, t_shell *shell)
{
	setup_child_process_signals(cmd);
	if (!setup_redirections(cmd, in_fd, out_fd))
	{
		free(exec_path);
		exit(1);
	}
	close_unneeded_fds(in_fd, out_fd);
	execute_child_command(cmd, exec_path, shell);
}
