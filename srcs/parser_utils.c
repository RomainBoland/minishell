/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser_utils.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: evan-dro <evan-dro@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/14 17:22:09 by evan-dro          #+#    #+#             */
/*   Updated: 2025/04/14 18:04:34 by evan-dro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

static	void heredoc_redirection(t_command *cmd, char *file)
{
	char	**new_delim;
	int		i;

	i = 0;
	cmd->has_heredoc = 1;
	new_delim = malloc(sizeof(char *) * (cmd->heredoc_count + 2));
	if (!new_delim)
		return;
	while (i < cmd->heredoc_count)
	{
		new_delim[i] = cmd->heredoc_delims[i];
		i++;
	}
	new_delim[i] = ft_strdup(file);
	if (!new_delim[i])
	{
		free(new_delim);
		return;
	}
	new_delim[i + 1] = NULL;
	if (cmd->heredoc_delims)
		free(cmd->heredoc_delims);
	cmd->heredoc_delims = new_delim;
	cmd->heredoc_count++;
}

void	add_redirection(t_command *cmd, char *file, int type)
{
	t_redirection	*new_redir;
	t_redirection	*current;

	new_redir = malloc(sizeof(t_redirection));
	if (!new_redir)
		return ;
	new_redir->file = ft_strdup(file);
	new_redir->type = type;
	new_redir->next = NULL;
	if (!cmd->redirections)
		cmd->redirections = new_redir;
	else
	{
		current = cmd->redirections;
		while (current->next)
			current = current->next;
		current->next = new_redir;
	}
	if (type == TOKEN_HEREDOC)
		heredoc_redirection(cmd, file);
}

static int	count_args(t_command *cmd)
{
	int	count;

	count = 0;
	if (cmd->args)
	{
		while (cmd->args[count])
			count++;
	}
	return (count);
}

// Add word to command arguments
void	add_arg(t_command *cmd, char *arg, int quoted_state)
{
	int		j;
	char	**new_args;
	int		*new_arg_quoted;
	int		count;

	if (!cmd || !arg)
		return;
	count = count_args(cmd);
	j = 0;
	new_args = malloc(sizeof(char *) * (count + 2));
	new_arg_quoted = malloc(sizeof(int) * (count + 2));
	if (!new_args || !new_arg_quoted)
	{
		if (new_args)
			free(new_args);
		if (new_arg_quoted)
			free(new_arg_quoted);
		return;
	}
	while (j < count)
	{
		new_args[j] = cmd->args[j];
		new_arg_quoted[j] = cmd->arg_quoted[j];
		j++;
	}
	new_args[count] = ft_strdup(arg);
	if (!new_args[count])
	{
		free(new_args);
		free(new_arg_quoted);
		return;
	}
	new_arg_quoted[count] = quoted_state;
	new_args[count + 1] = NULL;
	new_arg_quoted[count + 1] = 0;
		if (cmd->args)
		free(cmd->args);
	if (cmd->arg_quoted)
		free(cmd->arg_quoted);
	cmd->args = new_args;
	cmd->arg_quoted = new_arg_quoted;
}
