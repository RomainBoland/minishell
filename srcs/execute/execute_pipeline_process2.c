/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_pipeline_process2.c                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/25 09:49:36 by rboland           #+#    #+#             */
/*   Updated: 2025/04/25 09:49:36 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

/* Handle output redirections */
void	handle_output_redirections(t_command *cmd, int out_fd)
{
	t_redirection	*redir;
	int				output_redirected;
	int				fd;

	output_redirected = 0;
	redir = cmd->redirections;
	while (redir)
	{
		if (redir->type == TOKEN_REDIR_OUT)
		{
			fd = open(redir->file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (fd < 0)
			{
				perror(redir->file);
				exit(1);
			}
			dup2(fd, STDOUT_FILENO);
			close(fd);
			output_redirected = 1;
		}
		else if (redir->type == TOKEN_APPEND)
		{
			fd = open(redir->file, O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (fd < 0)
			{
				perror(redir->file);
				exit(1);
			}
			dup2(fd, STDOUT_FILENO);
			close(fd);
			output_redirected = 1;
		}
		redir = redir->next;
	}
	if (!output_redirected && out_fd != STDOUT_FILENO)
		dup2(out_fd, STDOUT_FILENO);
}

/* Close all pipe file descriptors in child */
void	close_child_pipes(int cmd_count, int i)
{
	(void)cmd_count;
	(void)i;
	/* Don't close stdin/stdout directly, they are handled through dup2() */
}

/* Execute child process */
void	execute_child_process(t_pipeline *pipeline, t_shell *shell, int i)
{
	t_command	*cmd;
	char		*exec_path;
	char		**env_array;

	cmd = pipeline->commands[i];
	if (is_builtin(cmd->args[0]))
		exit(execute_builtin(cmd, shell));
	else
	{
		exec_path = find_executable(cmd->args[0], shell);
		if (!exec_path)
		{
			ft_putstr_fd("minishell: command not found: ", 2);
			ft_putendl_fd(cmd->args[0], 2);
			exit(127);
		}
		env_array = env_to_array(shell->env);
		if (!env_array)
		{
			free(exec_path);
			exit(1);
		}
		execve(exec_path, cmd->args, env_array);
		perror(exec_path);
		free(exec_path);
		free_env_array(env_array);
		exit(1);
	}
}

/* Handle parent process pipes */
void	handle_parent_pipes(int i, int in_fd, int active_pipe,
		int pipefds[2][2], t_pipeline *pipeline)
{
	if (i > 0)
		close(in_fd);
	if (i < pipeline->cmd_count - 1)
		close(pipefds[active_pipe][1]);
}

/* Wait for processes */
void	wait_for_processes(pid_t *pids, int count)
{
	int	j;

	j = 0;
	while (j < count)
	{
		if (pids[j] > 0)
			waitpid(pids[j], NULL, 0);
		j++;
	}
}
