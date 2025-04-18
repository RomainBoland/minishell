/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/18 13:27:36 by rboland           #+#    #+#             */
/*   Updated: 2025/03/18 13:27:36 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

// Count number of commands in the pipeline
int count_commands(t_token *tokens)
{
    int count = 1;  // At least one command
    t_token *current = tokens;
    
    while (current)
    {
        if (current->type == TOKEN_PIPE)
            count++;
        current = current->next;
    }
    
    return count;
}

// Create a new command structure
t_command *create_command(t_token *tokens)
{
    t_command *cmd = malloc(sizeof(t_command));
    
    if (!cmd)
        return NULL;
        
    cmd->args = NULL;
    cmd->arg_quoted = NULL;
    cmd->redirections = NULL;
    cmd->has_heredoc = 0;
    cmd->heredoc_count = 0;
    cmd->heredoc_delims = NULL;
    cmd->heredoc_quoted = NULL;
    cmd->token = tokens;
    
    return cmd;
}

void add_redirection(t_command *cmd, char *file, int type, int quoted_state)
{
    t_redirection *new_redir;
    t_redirection *current;
    char **new_delim;
    int *new_quoted;

    new_delim = NULL;
    new_redir = malloc(sizeof(t_redirection));
    if (!new_redir)
        return;
    
    new_redir->file = ft_strdup(file);
    new_redir->type = type;
    new_redir->quoted = quoted_state; // Store if the heredoc delimiter was quoted
    new_redir->next = NULL;

    if (!cmd->redirections)
    {
        cmd->redirections = new_redir;
    }
    else
    {
        current = cmd->redirections;
        while (current->next)
            current = current->next;
        current->next = new_redir;
    }

    if (type == TOKEN_HEREDOC)
    {
        cmd->has_heredoc = 1;

        // Allocate new delimiters array
        new_delim = malloc(sizeof(char *) * (cmd->heredoc_count + 1));
        if (!new_delim)
            return;
        
        // Copy existing delimiters
        for (int i = 0; i < cmd->heredoc_count; i++)
            new_delim[i] = cmd->heredoc_delims[i];
        
        new_delim[cmd->heredoc_count] = ft_strdup(file);

        // Allocate new quoted state array
        new_quoted = malloc(sizeof(int) * (cmd->heredoc_count + 1));
        if (!new_quoted)
        {
            free(new_delim);
            return;
        }
        
        // Copy existing quoted states
        for (int i = 0; i < cmd->heredoc_count; i++)
            new_quoted[i] = cmd->heredoc_quoted[i];
        
        new_quoted[cmd->heredoc_count] = quoted_state;

        // Replace old arrays
        if (cmd->heredoc_delims)
            free(cmd->heredoc_delims);
        if (cmd->heredoc_quoted)
            free(cmd->heredoc_quoted);

        cmd->heredoc_delims = new_delim;
        cmd->heredoc_quoted = new_quoted;
        cmd->heredoc_count++;
    }
}

// Add word to command arguments
void add_arg(t_command *cmd, char *arg, int quoted_state)
{
    int i;
    int j;
    char **new_args;
    int *new_arg_quoted;

    i = 0;
    j = 0;
    
    // Count current args
    if (cmd->args)
    {
        while (cmd->args[i])
            i++;
    }
    
    // Allocate new array with one more slot
    new_args = malloc(sizeof(char *) * (i + 2));
    new_arg_quoted = malloc(sizeof(int) * (i + 2));
    if (!new_args || !new_arg_quoted)
        return;
        
    // Copy existing args
    while (j < i)
    {
        new_args[j] = cmd->args[j];
        new_arg_quoted[j] = cmd->arg_quoted[j];
        j++;
    }
        
    // Add new arg
    new_args[i] = ft_strdup(arg);
    new_arg_quoted[i] = quoted_state;
    new_args[i + 1] = NULL;
    new_arg_quoted[i + 1] = 0;
    
    // Free old array but not the strings
    free(cmd->args);
    free(cmd->arg_quoted);
    cmd->args = new_args;
    cmd->arg_quoted = new_arg_quoted;
}

// Parse tokens into a pipeline of commands
t_pipeline *parse_tokens(t_token *tokens)
{
    t_pipeline  *pipeline;
    t_token     *current;
    int         cmd_index;

    current = tokens;
    cmd_index = 0;
    
    // Create pipeline structure
    pipeline = malloc(sizeof(t_pipeline));
    if (!pipeline)
        return NULL;
        
    // Count commands and allocate array
    pipeline->cmd_count = count_commands(tokens);
    pipeline->commands = malloc(sizeof(t_command *) * pipeline->cmd_count);
    if (!pipeline->commands)
    {
        free(pipeline);
        return NULL;
    }
    
    // Initialize first command
    pipeline->commands[cmd_index] = create_command(tokens);
    
    // Process tokens
    while (current)
    {
        // Handle pipe - move to next command
        if (current->type == TOKEN_PIPE)
        {
            cmd_index++;
            pipeline->commands[cmd_index] = create_command(tokens);
            current = current->next;
            continue;
        }
        
		// Replace the existing redirection handling code with:
		if (current->type == TOKEN_REDIR_IN && current->next && current->next->type == TOKEN_WORD)
		{
			add_redirection(pipeline->commands[cmd_index], current->next->value, TOKEN_REDIR_IN, current->next->quoted_state);
			current = current->next->next;  // Skip redirection and filename
			continue;
		}

		if (current->type == TOKEN_REDIR_OUT && current->next && current->next->type == TOKEN_WORD)
		{
			add_redirection(pipeline->commands[cmd_index], current->next->value, TOKEN_REDIR_OUT, current->next->quoted_state);
			current = current->next->next;  // Skip redirection and filename
			continue;
		}

		if (current->type == TOKEN_APPEND && current->next && current->next->type == TOKEN_WORD)
		{
			add_redirection(pipeline->commands[cmd_index], current->next->value, TOKEN_APPEND, current->next->quoted_state);
			current = current->next->next;  // Skip redirection and filename
			continue;
		}

		if (current->type == TOKEN_HEREDOC && current->next && current->next->type == TOKEN_WORD)
        {
            add_redirection(pipeline->commands[cmd_index], current->next->value, TOKEN_HEREDOC, current->next->quoted_state);
            current = current->next->next;  // Skip redirection and delimiter
            continue;
        }
				
        // Handle normal words (command and arguments)
        if (current->type == TOKEN_WORD)
        {
            add_arg(pipeline->commands[cmd_index], current->value, current->quoted_state);
            current = current->next;
            continue;
        }
        
        // Skip unknown tokens
        current = current->next;
    }
    
    return pipeline;
}

// Free command structure
void free_command(t_command *cmd)
{
    t_redirection *redir;
    t_redirection *next_redir;
    
    if (!cmd)
        return;
        
    // Free args array and strings
    if (cmd->args)
    {
        for (int i = 0; cmd->args[i]; i++)
            free(cmd->args[i]);
        free(cmd->args);
    }
    
    if (cmd->arg_quoted)
        free(cmd->arg_quoted);
    
    // Free redirections
    redir = cmd->redirections;
    while (redir)
    {
        next_redir = redir->next;
        free(redir->file);
        free(redir);
        redir = next_redir;
    }
    
    // Free heredoc delimiter if it exists
    if (cmd->heredoc_delims)
    {
		for (int i = 0; i < cmd->heredoc_count; i++)
			free(cmd->heredoc_delims[i]);
		free(cmd->heredoc_delims);
	}

    if (cmd->heredoc_quoted)
        free(cmd->heredoc_quoted);
        
    free(cmd);
}

// Free pipeline structure
void free_pipeline(t_pipeline *pipeline)
{
    if (!pipeline)
        return;
        
    // Free all commands
    for (int i = 0; i < pipeline->cmd_count; i++)
        free_command(pipeline->commands[i]);
        
    free(pipeline->commands);
    free(pipeline);
}
