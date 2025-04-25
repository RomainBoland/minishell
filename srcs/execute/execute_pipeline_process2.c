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

/* Process a redirection of type REDIR_OUT */
int	process_out_redir(t_redirection *redir)
{
	int	fd;

	fd = open(redir->file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
	{
		perror(redir->file);
		return (0);
	}
	dup2(fd, STDOUT_FILENO);
	close(fd);
	return (1);
}

/* Process a redirection of type APPEND */
int	process_append_redir(t_redirection *redir)
{
	int	fd;

	fd = open(redir->file, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd < 0)
	{
		perror(redir->file);
		return (0);
	}
	dup2(fd, STDOUT_FILENO);
	close(fd);
	return (1);
}

/* Handle output redirections */
void	handle_output_redirections(t_command *cmd, int out_fd)
{
	t_redirection	*redir;
	int				output_redirected;

	output_redirected = 0;
	redir = cmd->redirections;
	while (redir)
	{
		if (redir->type == TOKEN_REDIR_OUT)
		{
			output_redirected = process_out_redir(redir);
			if (!output_redirected)
				exit(1);
		}
		else if (redir->type == TOKEN_APPEND)
		{
			output_redirected = process_append_redir(redir);
			if (!output_redirected)
				exit(1);
		}
		redir = redir->next;
	}
	if (!output_redirected && out_fd != STDOUT_FILENO)
		dup2(out_fd, STDOUT_FILENO);
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
