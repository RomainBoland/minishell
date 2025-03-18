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

// Create a new token
t_token *new_token(char *value, int type)
{
    t_token *token;
    
    token = malloc(sizeof(t_token));
    if (!token)
        return NULL;
        
    token->value = ft_strdup(value);
    token->type = type;
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
        printf("Warning: unclosed quote\n");
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

// Extract a word token
char *extract_word(char *input, int *i)
{
    int start = *i;
    int end = start;
    
    // Find end of word (space or special character)
    while (input[end] && !ft_strchr(" \t\n|<>", input[end]))
    {
        if (input[end] == '\'' || input[end] == '\"')
        {
            char quote = input[end];
            end++; // Skip opening quote
            while (input[end] && input[end] != quote)
                end++;
            if (input[end])
                end++; // Skip closing quote
        }
        else
            end++;
    }
    
    *i = end;
    
    // Extract the word
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
    
    while (input[i])
    {
        // Skip whitespace
        if (input[i] == ' ' || input[i] == '\t')
        {
            i++;
            continue;
        }
        
        // Handle quotes
        if (input[i] == '\'' || input[i] == '\"')
        {
            token_value = extract_quoted_str(input, &i, input[i]);
            add_token(&tokens, new_token(token_value, TOKEN_WORD));
            free(token_value);
            continue;
        }
        
        // Handle pipe
        if (input[i] == '|')
        {
            add_token(&tokens, new_token("|", TOKEN_PIPE));
            i++;
            continue;
        }
        
        // Handle input redirection
        if (input[i] == '<')
        {
            if (input[i + 1] == '<')
            {
                add_token(&tokens, new_token("<<", TOKEN_HEREDOC));
                i += 2;
            }
            else
            {
                add_token(&tokens, new_token("<", TOKEN_REDIR_IN));
                i++;
            }
            continue;
        }
        
        // Handle output redirection
        if (input[i] == '>')
        {
            if (input[i + 1] == '>')
            {
                add_token(&tokens, new_token(">>", TOKEN_APPEND));
                i += 2;
            }
            else
            {
                add_token(&tokens, new_token(">", TOKEN_REDIR_OUT));
                i++;
            }
            continue;
        }
        
        // Handle normal words
        token_value = extract_word(input, &i);
        add_token(&tokens, new_token(token_value, TOKEN_WORD));
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
