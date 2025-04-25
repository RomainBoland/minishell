/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_setup.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/25 09:46:36 by rboland           #+#    #+#             */
/*   Updated: 2025/04/25 09:46:36 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

/* Save and restore signal handlers for heredoc */
void	handle_heredoc_signals(struct sigaction *old_int, struct sigaction *old_quit)
{
	struct sigaction	sa_int;
	struct sigaction	sa_quit;

	sigaction(SIGINT, NULL, old_int);
	sigaction(SIGQUIT, NULL, old_quit);
	ft_memset(&sa_int, 0, sizeof(sa_int));
	ft_memset(&sa_quit, 0, sizeof(sa_quit));
	sa_int.sa_handler = SIG_IGN;
	sa_quit.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sa_int, NULL);
	sigaction(SIGQUIT, &sa_quit, NULL);
}

/* Setup child process signal handling for heredoc */
void	setup_child_signals(void)
{
	struct sigaction	sa_child_int;

	ft_memset(&sa_child_int, 0, sizeof(sa_child_int));
	sa_child_int.sa_handler = SIG_DFL;
	sigaction(SIGINT, &sa_child_int, NULL);
}

/* Process heredoc lines */
void	process_heredoc_lines(t_command *cmd, int pipe_fd[2], t_shell *shell)
{
	char	*line;
	int		i;
	
	close(pipe_fd[0]);
	for (i = 0; i < cmd->heredoc_count; i++)
	{
		char *collected_input = ft_strdup("");
		while (1)
		{
			write(STDOUT_FILENO, "> ", 2);
			line = readline("");
			if (!line || ft_strcmp(line, cmd->heredoc_delims[i]) == 0)
			{
				free(line);
				break;
			}
			if (i == cmd->heredoc_count - 1)
			{
				collected_input = append_heredoc_line(collected_input, line, 
					cmd->heredoc_quoted[i], shell);
			}
			free(line);
		}
		if (i == cmd->heredoc_count - 1)
			write(pipe_fd[1], collected_input, ft_strlen(collected_input));
		
		free(collected_input);
	}
	close(pipe_fd[1]);
	exit(0);
}

/* Append line to heredoc collected input */
char	*append_heredoc_line(char *collected, char *line, int quoted, t_shell *shell)
{
	char	*expanded;
	char	*temp;
	char	*result;

	if (!quoted)
	{
		expanded = expand_variables(line, shell);
		if (expanded)
		{
			temp = collected;
			result = ft_strjoin(collected, expanded);
			free(temp);
			temp = result;
			result = ft_strjoin(result, "\n");
			free(temp);
			free(expanded);
			return (result);
		}
	}
	temp = collected;
	result = ft_strjoin(collected, line);
	free(temp);
	temp = result;
	result = ft_strjoin(result, "\n");
	free(temp);
	return (result);
}
