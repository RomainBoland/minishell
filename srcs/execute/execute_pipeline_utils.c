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

/* Execute a single command in the pipeline */
int	execute_pipeline_command(t_pipeline *pipeline, t_shell *shell, 
		int i, int *active_pipe, int pipefds[2][2], int in_fd, 
		pid_t *pids, int *heredoc_fds)
{
	int	out_fd;

	/* Determine output destination */
	if (i == pipeline->cmd_count - 1)
		out_fd = STDOUT_FILENO;
	else
	{
		out_fd = pipefds[*active_pipe][1];
		/* Create pipe for next command if needed */
		if (i < pipeline->cmd_count - 2)
		{
			if (pipe(pipefds[1 - *active_pipe]) < 0)
			{
				handle_pipe_error(heredoc_fds, pipeline->cmd_count);
				return (1);
			}
		}
	}

	/* Create child process */
	pids[i] = fork();
	if (pids[i] < 0)
	{
		perror("fork");
		return (1);
	}

	/* Child process execution */
	if (pids[i] == 0)
	{
		/* Close all pipe ends except those needed for this command */
		close_unused_pipes(pipeline, i, *active_pipe, pipefds);
		/* Execute the command */
		setup_pipe_redirects(in_fd, out_fd);
		execute_pipeline_command_child(pipeline, shell, i, heredoc_fds);
	}

	/* Parent process */
	handle_parent_pipes_fixed(i, in_fd, *active_pipe, pipefds, pipeline);
	*active_pipe = 1 - *active_pipe;
	return (0);
}

/* Close unused pipes in child process */
void	close_unused_pipes(t_pipeline *pipeline, int i, int active_pipe, 
		int pipefds[2][2])
{
	/* Close read end of current pipe if not last command */
	if (i < pipeline->cmd_count - 1)
		close(pipefds[active_pipe][0]);

	/* Close both ends of next pipe if not second-to-last command */
	if (i < pipeline->cmd_count - 2)
	{
		close(pipefds[1 - active_pipe][0]);
		close(pipefds[1 - active_pipe][1]);
	}

	/* Close previous pipe write end if not first command */
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

/* Execute command in pipeline child process */
void	execute_pipeline_command_child(t_pipeline *pipeline, t_shell *shell,
		int i, int *heredoc_fds)
{
	t_command	*cmd;
	char		*exec_path;
	char		**env_array;

	cmd = pipeline->commands[i];
	/* Setup heredoc if needed */
	if (cmd->has_heredoc && heredoc_fds[i] != STDIN_FILENO)
	{
		dup2(heredoc_fds[i], STDIN_FILENO);
		close(heredoc_fds[i]);
	}

	/* Close all other heredoc FDs */
	cleanup_heredocs(heredoc_fds, pipeline->cmd_count);
	free(heredoc_fds);

	/* Setup command redirections */
	if (!setup_cmd_redirections(cmd))
		exit(1);

	/* Execute command */
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

/* Handle parent process pipes with improved logic */
void	handle_parent_pipes_fixed(int i, int in_fd, int active_pipe,
		int pipefds[2][2], t_pipeline *pipeline)
{
	/* Close input pipe if not first command */
	if (i > 0)
		close(in_fd);
	
	/* Close write end of current pipe */
	if (i < pipeline->cmd_count - 1)
		close(pipefds[active_pipe][1]);
}
