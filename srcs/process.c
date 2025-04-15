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

// Check if a token is a redirection operator
int is_redirection(int token_type)
{
    return (token_type == TOKEN_REDIR_IN || 
            token_type == TOKEN_REDIR_OUT || 
            token_type == TOKEN_APPEND || 
            token_type == TOKEN_HEREDOC);
}

// Validate syntax of tokens
int validate_syntax(t_token *tokens)
{
    t_token *current = tokens;
    t_token *prev = NULL;
    int cmd_found = 0; // Track if we found at least one command
    
    // Empty input is valid
    if (!current)
        return 1;
    
    while (current)
    {
        // Check for pipe errors
        if (current->type == TOKEN_PIPE)
        {
            // Error: Pipe at beginning or after another pipe
            if (!prev || prev->type == TOKEN_PIPE)
            {
                ft_putendl_fd("minishell: syntax error near unexpected token `|'", STDERR_FILENO);
                return 0;
            }
            
            // Error: Pipe followed by nothing
            if (!current->next)
            {
                ft_putendl_fd("minishell: syntax error near unexpected token `|'", STDERR_FILENO);
                return 0;
            }
        }
        
        // Check for redirection errors
        if (is_redirection(current->type))
        {
            // Error: Redirection not followed by a word
            if (!current->next || current->next->type != TOKEN_WORD)
            {
                if (!current->next)
                {
                    ft_putendl_fd("minishell: syntax error near unexpected token `newline'", STDERR_FILENO);
                    return 0;
                }
                else if (current->next->type == TOKEN_PIPE)
                {
                    ft_putendl_fd("minishell: syntax error near unexpected token `|'", STDERR_FILENO);
                    return 0;
                }
                else if (is_redirection(current->next->type))
                {
                    // Determine which redirection is causing the error
                    const char *token_str = NULL;
                    
                    if (current->next->type == TOKEN_REDIR_IN)
                        token_str = "<";
                    else if (current->next->type == TOKEN_REDIR_OUT)
                        token_str = ">";
                    else if (current->next->type == TOKEN_APPEND)
                        token_str = ">>";
                    else if (current->next->type == TOKEN_HEREDOC)
                        token_str = "<<";
                        
                    ft_putstr_fd("minishell: syntax error near unexpected token `", STDERR_FILENO);
                    ft_putstr_fd((char*)token_str, STDERR_FILENO);
                    ft_putendl_fd("'", STDERR_FILENO);
                    return 0;
                }
            }
        }
        
        // Track if we've found a command
        if (current->type == TOKEN_WORD && (!prev || prev->type == TOKEN_PIPE || 
            (is_redirection(prev->type) && prev->next && prev->next->next == current)))
        {
            cmd_found = 1;
        }
        
        prev = current;
        current = current->next;
    }
    
    // Make sure we found at least one command or word
    if (!cmd_found)
    {
        // The input consists only of redirections without commands
        // This is actually valid in Bash (e.g., "> file" creates/truncates file)
        // But if we want to enforce at least one command, we can uncomment:
        // ft_putendl_fd("minishell: syntax error: command expected", STDERR_FILENO);
        // return 0;
    }
    
    return 1;
}

// Add this to validate multiple consecutive redirections
int check_consecutive_redirections(t_token *tokens)
{
    t_token *current = tokens;
    
    while (current && current->next)
    {
        if (is_redirection(current->type) && is_redirection(current->next->type))
        {
            // Specific error message for the second redirection token
            const char *token_str = NULL;
            
            if (current->next->type == TOKEN_REDIR_IN)
                token_str = "<";
            else if (current->next->type == TOKEN_REDIR_OUT)
                token_str = ">";
            else if (current->next->type == TOKEN_APPEND)
                token_str = ">>";
            else if (current->next->type == TOKEN_HEREDOC)
                token_str = "<<";
                
            ft_putstr_fd("minishell: syntax error near unexpected token `", STDERR_FILENO);
            ft_putstr_fd((char*)token_str, STDERR_FILENO);
            ft_putendl_fd("'", STDERR_FILENO);
            return 0;
        }
        current = current->next;
    }
    
    return 1;
}

// Process the input line
void process_input(char *input, t_shell *shell)
{
    t_token     *tokens;
    t_pipeline  *pipeline;
    int         status;
    
    // Tokenize input
    tokens = tokenize_input(input);
    if (!tokens)
        return;
    
    // Validate syntax before proceeding
    if (!validate_syntax(tokens) || !check_consecutive_redirections(tokens))
    {
        // Set last exit status to 2 (syntax error in bash)
        shell->last_exit_status = 2;
        free_tokens(tokens);
        return;
    }
    
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
    
    // Execute the pipeline
    status = execute_pipeline(pipeline, shell);
    shell->last_exit_status = status;
    
    setup_signals();
    // Clean up
    free_tokens(tokens);
    free_pipeline(pipeline);
}
