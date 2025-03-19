/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/18 13:28:31 by rboland           #+#    #+#             */
/*   Updated: 2025/03/18 13:28:31 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

// Debug function to print tokens
void print_tokens(t_token *tokens)
{
    t_token *current = tokens;
    
    printf("Tokens:\n");
    while (current)
    {
        printf("  Type: %d, Value: '%s'\n", current->type, current->value);
        current = current->next;
    }
}

// Debug function to print pipeline
void print_pipeline(t_pipeline *pipeline)
{
    printf("Pipeline with %d commands:\n", pipeline->cmd_count);
    
    for (int i = 0; i < pipeline->cmd_count; i++)
    {
        t_command *cmd = pipeline->commands[i];
        
        printf("Command %d:\n", i + 1);
        
        printf("  Args: ");
        if (cmd->args)
        {
            for (int j = 0; cmd->args[j]; j++)
                printf("'%s' ", cmd->args[j]);
        }
        printf("\n");
        
        if (cmd->input_file)
            printf("  Input from: %s\n", cmd->input_file);
        if (cmd->heredoc_delim)
            printf("  Heredoc delimiter: %s\n", cmd->heredoc_delim);
        if (cmd->output_file)
            printf("  Output to: %s (append: %d)\n", cmd->output_file, cmd->append_output);
    }
}

// Process the input line
void process_input(char *input, t_shell *shell)
{
    t_token *tokens;
    t_pipeline *pipeline;
    
    // Tokenize input
    tokens = tokenize_input(input);
    if (!tokens)
    {
        printf("Error: Failed to tokenize input\n");
        return;
    }
    
    // Debug: print tokens
	// printf("	\033[31mTokens:\033[0m\n\n");
    // print_tokens(tokens);
    
    // Parse tokens into pipeline
    pipeline = parse_tokens(tokens);
    if (!pipeline)
    {
        printf("Error: Failed to parse tokens\n");
        free_tokens(tokens);
        return;
    }
    
	// Debug: print commands
	// printf("	\033[31mCommands:\033[0m\n\n");
	// print_pipeline(pipeline);
	// printf("	\033[31mResult:\033[0m\n\n");

    // Execute the pipeline
    int status = execute_pipeline(pipeline, shell);
    
    // Update the $? variable
    shell->last_exit_status = status;
        
    // Clean up
    free_tokens(tokens);
    free_pipeline(pipeline);
}
