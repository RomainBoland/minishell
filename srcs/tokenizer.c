/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tokenizer.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/18 13:27:08 by rboland           #+#    #+#             */
/*   Updated: 2025/03/18 13:27:08 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

// Create a new token with quote state
t_token *new_token(char *value, int type, int quoted_state)
{
    t_token *token;
    
    token = malloc(sizeof(t_token));
    if (!token)
        return NULL;
        
    token->value = ft_strdup(value);
    token->type = type;
    token->quoted_state = quoted_state;
    token->next = NULL;
    
    return token;
}

// Add token to the end of the list
void add_token(t_token **tokens, t_token *new)
{
    t_token *current;
    
    if (!*tokens)
    {
        *tokens = new;
        return;
    }
    
    current = *tokens;
    while (current->next)
        current = current->next;
        
    current->next = new;
}

// Handle quotes (both single and double)
char *extract_quoted_str(char *input, int *i, char quote_char)
{
    int start = *i + 1;
    int end = start;
    
    // Find closing quote
    while (input[end] && input[end] != quote_char)
        end++;
        
    // If no closing quote found, consider it until end of string
    if (!input[end])
    {
        *i = end;
        return ft_strdup(&input[start]);
    }
    
    *i = end + 1;  // Move past the closing quote
    
    // Extract the quoted content
    char *result = malloc(end - start + 1);
    if (!result)
        return NULL;
        
    ft_strlcpy(result, &input[start], end - start + 1);
    
    return result;
}

// Extract a word token with proper quote handling
char *extract_word(char *input, int *i)
{
    int start = *i;
    int end = start;
    int in_single_quotes = 0;
    int in_double_quotes = 0;
    
    // Find end of word (space or special character outside quotes)
    while (input[end])
    {
        // Handle quotes
        if (input[end] == '\'' && !in_double_quotes)
        {
            in_single_quotes = !in_single_quotes;
            end++;
            continue;
        }
        else if (input[end] == '\"' && !in_single_quotes)
        {
            in_double_quotes = !in_double_quotes;
            end++;
            continue;
        }
        
        // Stop at unquoted spaces or special chars
        if (!in_single_quotes && !in_double_quotes && 
            ft_strchr(" \t\n|<>", input[end]))
            break;
            
        end++;
    }
    
    *i = end;
    
    // Extract the word with quotes preserved
    char *result = malloc(end - start + 1);
    if (!result)
        return NULL;
        
    ft_strlcpy(result, &input[start], end - start + 1);
    
    return result;
}

// Main tokenization function
t_token *tokenize_input(char *input)
{
    t_token *tokens = NULL;
    int i = 0;
    char *token_value;

    if (has_unclosed_quotes(input))
    {
        ft_putendl_fd("minishell: syntax error: unclosed quotes", STDERR_FILENO);
        return NULL;
    }
    
    
    while (input[i])
    {
        // Skip whitespace
        if (input[i] == ' ' || input[i] == '\t')
        {
            i++;
            continue;
        }
        
        // Handle pipe
        if (input[i] == '|')
        {
            add_token(&tokens, new_token("|", TOKEN_PIPE, 0));
            i++;
            continue;
        }
        
        // Handle input redirection
        if (input[i] == '<')
        {
            if (input[i + 1] == '<') // This is a heredoc (<<)
            {
                if (input[i + 2] == '<') // More than 2 consecutive < is an error
                {
                    ft_putendl_fd("minishell: syntax error near unexpected token `<'", STDERR_FILENO);
                    free_tokens(tokens);
                    return NULL;
                }

                // Add heredoc token
                add_token(&tokens, new_token("<<", TOKEN_HEREDOC, 0));
                i += 2;
                
                // Skip whitespace after << token
                while (input[i] == ' ' || input[i] == '\t')
                    i++;
                
                // Now check if the delimiter is quoted
                if (input[i] == '"') // Double-quoted delimiter
                {
                    i++; // Skip opening double quote
                    
                    // Find the closing quote
                    int start = i;
                    while (input[i] && input[i] != '"')
                        i++;
                    
                    // Create delimiter token (without quotes)
                    char *delim = malloc(i - start + 1);
                    if (delim)
                    {
                        ft_strlcpy(delim, &input[start], i - start + 1);
                        add_token(&tokens, new_token(delim, TOKEN_WORD, 2)); // 2 = double quoted
                        free(delim);
                    }
                    
                    if (input[i] == '"')
                        i++; // Skip closing quote
                }
                else if (input[i] == '\'') // Single-quoted delimiter
                {
                    i++; // Skip opening single quote
                    
                    // Find the closing quote
                    int start = i;
                    while (input[i] && input[i] != '\'')
                        i++;
                    
                    // Create delimiter token (without quotes)
                    char *delim = malloc(i - start + 1);
                    if (delim)
                    {
                        ft_strlcpy(delim, &input[start], i - start + 1);
                        add_token(&tokens, new_token(delim, TOKEN_WORD, 1)); // 1 = single quoted
                        free(delim);
                    }
                    
                    if (input[i] == '\'')
                        i++; // Skip closing quote
                }
                else // Unquoted delimiter
                {
                    // Use the existing word extraction for unquoted text
                    char *delim = extract_word(input, &i);
                    if (delim)
                    {
                        add_token(&tokens, new_token(delim, TOKEN_WORD, 0)); // 0 = unquoted
                        free(delim);
                    }
                }
                continue;
            }
            else // This is a regular input redirection (<)
            {
                add_token(&tokens, new_token("<", TOKEN_REDIR_IN, 0));
                i++;
            }
            continue;
        }
        
        // Handle output redirection
        if (input[i] == '>')
        {
            if (input[i + 1] == '>')
            {
                if (input[i + 2] == '>')
                {
                    ft_putendl_fd("minishell: syntax error near unexpected token `>'", STDERR_FILENO);
                    free_tokens(tokens);
                    return NULL;
                }
                add_token(&tokens, new_token(">>", TOKEN_APPEND, 0));
                i += 2;
            }
            else
            {
                add_token(&tokens, new_token(">", TOKEN_REDIR_OUT, 0));
                i++;
            }
            continue;
        }
        
        // Handle normal words (including quoted strings)
        token_value = extract_word(input, &i);
        add_token(&tokens, new_token(token_value, TOKEN_WORD, 0)); // We'll handle quotes in expand_variables
        free(token_value);
    }
    
    return tokens;
}

// Free token list
void free_tokens(t_token *tokens)
{
    t_token *tmp;
    
    while (tokens)
    {
        tmp = tokens;
        tokens = tokens->next;
        free(tmp->value);
        free(tmp);
    }
}
