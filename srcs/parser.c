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
t_command *create_command(void)
{
    t_command *cmd = malloc(sizeof(t_command));
    
    if (!cmd)
        return NULL;
        
    cmd->args = NULL;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_output = 0;
    cmd->heredoc_delim = NULL;
    
    return cmd;
}

// Add word to command arguments
void add_arg(t_command *cmd, char *arg)
{
    int i = 0;
    char **new_args;
    
    // Count current args
    if (cmd->args)
    {
        while (cmd->args[i])
            i++;
    }
    
    // Allocate new array with one more slot
    new_args = malloc(sizeof(char *) * (i + 2));
    if (!new_args)
        return;
        
    // Copy existing args
    for (int j = 0; j < i; j++)
        new_args[j] = cmd->args[j];
        
    // Add new arg
    new_args[i] = ft_strdup(arg);
    new_args[i + 1] = NULL;
    
    // Free old array but not the strings
    free(cmd->args);
    cmd->args = new_args;
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
    pipeline->commands[cmd_index] = create_command();
    
    // Process tokens
    while (current)
    {
        // Handle pipe - move to next command
        if (current->type == TOKEN_PIPE)
        {
            cmd_index++;
            pipeline->commands[cmd_index] = create_command();
            current = current->next;
            continue;
        }
        
        // Handle redirections
        if (current->type == TOKEN_REDIR_IN && current->next && current->next->type == TOKEN_WORD)
        {
            pipeline->commands[cmd_index]->input_file = ft_strdup(current->next->value);
            current = current->next->next;  // Skip redirection and filename
            continue;
        }
        
        if (current->type == TOKEN_REDIR_OUT && current->next && current->next->type == TOKEN_WORD)
        {
            pipeline->commands[cmd_index]->output_file = ft_strdup(current->next->value);
            pipeline->commands[cmd_index]->append_output = 0;
            current = current->next->next;  // Skip redirection and filename
            continue;
        }
        
        if (current->type == TOKEN_APPEND && current->next && current->next->type == TOKEN_WORD)
        {
            pipeline->commands[cmd_index]->output_file = ft_strdup(current->next->value);
            pipeline->commands[cmd_index]->append_output = 1;
            current = current->next->next;  // Skip redirection and filename
            continue;
        }
        
        if (current->type == TOKEN_HEREDOC && current->next && current->next->type == TOKEN_WORD)
        {
            pipeline->commands[cmd_index]->heredoc_delim = ft_strdup(current->next->value);
            current = current->next->next;  // Skip redirection and delimiter
            continue;
        }
        
        // Handle normal words (command and arguments)
        if (current->type == TOKEN_WORD)
        {
            add_arg(pipeline->commands[cmd_index], current->value);
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
    if (!cmd)
        return;
        
    // Free args array and strings
    if (cmd->args)
    {
        for (int i = 0; cmd->args[i]; i++)
            free(cmd->args[i]);
        free(cmd->args);
    }
    
    // Free redirection strings
    if (cmd->input_file)
        free(cmd->input_file);
    if (cmd->output_file)
        free(cmd->output_file);
    if (cmd->heredoc_delim)
        free(cmd->heredoc_delim);
        
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
