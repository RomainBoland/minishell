/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   syntax_validation.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <romain.boland@hotmail.com>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/24 13:41:09 by rboland           #+#    #+#             */
/*   Updated: 2025/04/24 13:41:12 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

/* Check if a token represents a command */
static int	is_command_token(t_token *current, t_token *prev)
{
	if (current->type == TOKEN_WORD && (!prev || prev->type == TOKEN_PIPE || 
		(is_redirection(prev->type) && prev->next && prev->next->next == current)))
		return (1);
	return (0);
}

/* Check redirection syntax */
static int	check_redirection_syntax(t_token *current)
{
	if (!current->next || current->next->type != TOKEN_WORD)
		return (handle_missing_word_error(current));
	return (1);
}

// Validate syntax of tokens
int	validate_syntax(t_token *tokens)
{
	t_token	*current;
	t_token	*prev;
	int		cmd_found;
	
	current = tokens;
	prev = NULL;
	cmd_found = 0;
	
	if (!current)
		return (1);
	
	while (current)
	{
		if (current->type == TOKEN_PIPE)
		{
			if (!check_pipe_errors(current, prev))
				return (0);
		}
		
		if (is_redirection(current->type))
		{
			if (!check_redirection_syntax(current))
				return (0);
		}
		
		if (is_command_token(current, prev))
			cmd_found = 1;
		
		prev = current;
		current = current->next;
	}
	
	return (1);
}

// Check for consecutive redirections
int	check_consecutive_redirections(t_token *tokens)
{
	t_token	*current;
	
	current = tokens;
	
	while (current && current->next)
	{
		if (is_redirection(current->type) && is_redirection(current->next->type))
		{
			print_redir_error(current->next->type);
			return (0);
		}
		current = current->next;
	}
	
	return (1);
}
