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
        t_redirection *redir;
        
        printf("Command %d:\n", i + 1);
        
        printf("  Args: ");
        if (cmd->args)
        {
            for (int j = 0; cmd->args[j]; j++)
                printf("'%s' ", cmd->args[j]);
        }
        printf("\n");
        
        // Print redirections
        printf("  Redirections:\n");
        redir = cmd->redirections;
        while (redir)
        {
            if (redir->type == TOKEN_REDIR_IN)
                printf("    < %s\n", redir->file);
            else if (redir->type == TOKEN_REDIR_OUT)
                printf("    > %s\n", redir->file);
            else if (redir->type == TOKEN_APPEND)
                printf("    >> %s\n", redir->file);
            else if (redir->type == TOKEN_HEREDOC)
                printf("    << %s\n", redir->file);
            
            redir = redir->next;
        }
        
        if (cmd->heredoc_delims)
		{
			for (int i = 0; i < cmd->heredoc_count; i++)
				printf("  Heredoc delimiter: %s\n", cmd->heredoc_delims[i]);
		}
    }
}

// Process the input line
void process_input(char *input, t_shell *shell)
{
    t_token		*tokens;
    t_pipeline *pipeline;
	int			status;
    
    // Tokenize input
    tokens = tokenize_input(input);
    if (!tokens)
    {
        printf("Error: Failed to tokenize input\n");
        return;
    }
    
    // Debug: print tokens
    // print_tokens(tokens);
    
    // Parse tokens into pipeline
    pipeline = parse_tokens(tokens);
    if (!pipeline)
    {
        printf("Error: Failed to parse tokens\n");
        free_tokens(tokens);
        return;
    }
    
    // Expand environment variables
    expand_pipeline(pipeline, shell);
    
    // Debug: print pipeline
    // print_pipeline(pipeline);
    
    // Execute the pipeline
    status = execute_pipeline(pipeline, shell);
	shell->last_exit_status = status;
    
    // Update the $? variable (we'll implement this later with environment handling)
    // set_last_exit_status(status);
    
    // For debugging, you can still print the pipeline
    // print_pipeline(pipeline);
    
    // Clean up
    free_tokens(tokens);
    free_pipeline(pipeline);
}
