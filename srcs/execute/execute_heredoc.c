/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_heredoc.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/25 09:47:09 by rboland           #+#    #+#             */
/*   Updated: 2025/04/25 09:47:09 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

/* Setup heredoc for command */
int	setup_heredoc(t_command *cmd, t_shell *shell)
{
	int					pipe_fd[2];
	pid_t				pid;
	struct sigaction	old_int;
	struct sigaction	old_quit;

	if (!cmd->has_heredoc || cmd->heredoc_count == 0)
		return (STDIN_FILENO);
	if (pipe(pipe_fd) < 0)
	{
		perror("pipe");
		return (-1);
	}
	handle_heredoc_signals(&old_int, &old_quit);
	pid = fork();
	if (pid < 0)
		return (handle_heredoc_fork_error(pipe_fd, &old_int, &old_quit));
	if (pid == 0)
	{
		setup_child_signals();
		process_heredoc_lines(cmd, pipe_fd, shell);
	}
	return (handle_heredoc_parent(pid, pipe_fd, &old_int, &old_quit));
}

/* Handle heredoc fork error */
int	handle_heredoc_fork_error(int pipe_fd[2], struct sigaction *old_int, 
			struct sigaction *old_quit)
{
	perror("fork");
	close(pipe_fd[0]);
	close(pipe_fd[1]);
	sigaction(SIGINT, old_int, NULL);
	sigaction(SIGQUIT, old_quit, NULL);
	return (-1);
}

/* Handle heredoc parent process */
int	handle_heredoc_parent(pid_t pid, int pipe_fd[2], 
			struct sigaction *old_int, struct sigaction *old_quit)
{
	int	status;

	close(pipe_fd[1]);
	waitpid(pid, &status, 0);
	sigaction(SIGINT, old_int, NULL);
	sigaction(SIGQUIT, old_quit, NULL);
	if (WIFSIGNALED(status))
	{
		if (WTERMSIG(status) == SIGINT)
			rl_on_new_line();
		close(pipe_fd[0]);
		return (-1);
	}
	return (pipe_fd[0]);
}

/* Execute a command */
int	execute_command(t_command *cmd, int in_fd, int out_fd, t_shell *shell)
{
	int		heredoc_fd;

	if (!cmd || !cmd->args || !cmd->args[0])
		return (0);
	heredoc_fd = handle_command_heredoc(cmd, shell);
	if (heredoc_fd < 0)
		return (1);
	in_fd = update_input_fd(cmd, in_fd, heredoc_fd);
	if (is_builtin(cmd->args[0]))
		return (execute_builtin_with_redirects(cmd, in_fd, out_fd, shell));
	else
		return (execute_external_command(cmd, in_fd, out_fd, shell, heredoc_fd));
}

/* Handle command heredoc */
int	handle_command_heredoc(t_command *cmd, t_shell *shell)
{
	int	heredoc_fd;

	heredoc_fd = STDIN_FILENO;
	if (cmd->has_heredoc)
	{
		heredoc_fd = setup_heredoc(cmd, shell);
		if (heredoc_fd < 0)
			return (-1);
	}
	return (heredoc_fd);
}
