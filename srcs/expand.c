/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/24 13:39:56 by rboland           #+#    #+#             */
/*   Updated: 2025/03/24 13:39:56 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

// check if a character is valid in a variable name
static int	is_valid_var_char(char c, int first_char)
{
	if (first_char)
		return (ft_isalpha(c) || c == '_');
	else
		return (ft_isalnum(c) || c == '_');		
}

// Extract a variable name from a string starting at index 1
static char	*extract_var_name(char *str, int *i)
{
	int		start;
	int		len;
	char	*var_name;

	start = *i;
	len = 0;
	
	// skip the $ char
	(*i)++;

	// Special case for $?
	if (str[*i] == '?')
	{
		(*i)++;
		return (ft_strdup("?"));
	}

	// find len
	while (str[*i] && is_valid_var_char(str[*i], len == 0))
	{
		(*i)++;
		len++;
	}

	// no valid variable name found
	if (len == 0)
		return (NULL);
	
	// extract the variable name
	var_name = malloc(len + 1);
	if (!var_name)
		return (NULL);
	
	ft_strlcpy(var_name, (str + start + 1), (len + 1));
	return (var_name);
}

static char	*expand_single_var(char *str, int *i, t_shell *shell)
{
	char	*var_name;
	char	*var_value;

	// Extract variable name
	var_name = extract_var_name(str, i);
	if (!var_name)
		return (ft_strdup("")); // Empty variable name expands to empty string
	
	// Special case for $?
	if (!ft_strncmp(var_name, "?", 2))
	{
		free(var_name);
		return (ft_itoa(shell->last_exit_status));
	}

	// Get variable value from environment
	var_value = get_env_value(shell->env, var_name);
	free(var_name);

	// Return empty string if variable is not found 
	if (!var_value)
		return (ft_strdup(""));

	return (ft_strdup(var_value));
}

// Expand all environment variables in a string
char *expand_variables(char *str, t_shell *shell)
{
    int i = 0;
    char *result;
    char *temp;
    char *var_value;

	//printf("%s\n", str);
    
    // Initialize result as empty string
    result = ft_strdup("");
    if (!result)
        return NULL;
    
    // Process the string with quote removal
    while (str[i])
    {
        // Single quotes - everything inside is literal, but remove the quotes
        if (str[i] == '\'')
        {
            i++; // Skip the opening quote
            
            // Copy everything inside the single quotes literally
            while (str[i] && str[i] != '\'')
            {
                char c[2] = {str[i], '\0'};
                temp = result;
                result = ft_strjoin(result, c);
                free(temp);
                i++;
            }
            
            if (str[i] == '\'')
                i++; // Skip the closing quote
        }
        // Double quotes - expand variables inside, but remove the quotes
        else if (str[i] == '\"')
        {
            i++; // Skip the opening quote
            
            // Process content inside double quotes
            while (str[i] && str[i] != '\"')
            {
                // Expand variables inside double quotes
                if (str[i] == '$' && str[i+1] && 
                    (ft_isalnum(str[i+1]) || str[i+1] == '_' || str[i+1] == '?'))
                {
                    var_value = expand_single_var(str, &i, shell);
                    
                    temp = result;
                    result = ft_strjoin(result, var_value);
                    free(temp);
                    free(var_value);
                }
                else
                {
                    // Regular character inside double quotes
                    char c[2] = {str[i], '\0'};
                    temp = result;
                    result = ft_strjoin(result, c);
                    free(temp);
                    i++;
                }
            }
            
            if (str[i] == '\"')
                i++; // Skip the closing quote
        }
        // Handle variable expansion outside quotes
        else if (str[i] == '$' && str[i+1] && 
                 (ft_isalnum(str[i+1]) || str[i+1] == '_' || str[i+1] == '?'))
        {
            var_value = expand_single_var(str, &i, shell);
            
            temp = result;
            result = ft_strjoin(result, var_value);
            free(temp);
            free(var_value);
        }
        else
        {
            // Regular character outside quotes
            char c[2] = {str[i], '\0'};
            temp = result;
            result = ft_strjoin(result, c);
            free(temp);
            i++;
        }
    }
    
    return result;
}

// Check if a position in a string is inside double quotes
int	is_in_dquotes(char *str, int pos)
{
	int	i;
	int	in_dquotes;

	i = 0;
	in_dquotes = 0;

	while (i < pos)
	{
		if (str[i] == '"' && (i == 0 || str[i - 1] != '\\'))
			in_dquotes = !in_dquotes;
		i++;
	}

	return (in_dquotes);
}

// Expand variables in a command's arguments
void expand_command_args(t_command *cmd, t_shell *shell)
{
    int i;
    char *expanded;
    t_redirection *redir;

    if (!cmd || !cmd->args)
        return;

    // Expand command arguments
    i = 0;
    while (cmd->args[i])
    {
        // Skip expansion for single-quoted strings
        if (cmd->arg_quoted[i] == 1) // 1 = single quoted
        {
            i++;
            continue;
        }
        
        // For double-quoted or unquoted, expand variables
        expanded = expand_variables(cmd->args[i], shell);
        if (expanded)
        {
            free(cmd->args[i]);
            cmd->args[i] = expanded;
        }
        i++;
    }

    // Expand redirect filenames
    redir = cmd->redirections;
    while (redir)
    {
        // Don't expand filenames in heredocs
        if (redir->type != TOKEN_HEREDOC)
        {
            expanded = expand_variables(redir->file, shell);
            if (expanded)
            {
                free(redir->file);
                redir->file = expanded;
            }
        }
        redir = redir->next;
    }
    
    // Make sure heredoc_delim is also expanded if needed
    for (i = 0; i < cmd->heredoc_count; i++)
    {
        if (cmd->heredoc_delims[i])
        {
            expanded = expand_variables(cmd->heredoc_delims[i], shell);
            if (expanded)
            {
                free(cmd->heredoc_delims[i]);
                cmd->heredoc_delims[i] = expanded;
            }
        }
    }
}

// Expand variables in a pipeline of commands
void expand_pipeline(t_pipeline *pipeline, t_shell *shell)
{
    int i, j;
    char *expanded;
    t_redirection *redir;
    
    if (!pipeline)
        return;
    
    for (i = 0; i < pipeline->cmd_count; i++)
    {
        // Expand command arguments
        for (j = 0; pipeline->commands[i]->args && pipeline->commands[i]->args[j]; j++)
        {
            expanded = expand_variables(pipeline->commands[i]->args[j], shell);
            if (expanded)
            {
                free(pipeline->commands[i]->args[j]);
                pipeline->commands[i]->args[j] = expanded;
            }
        }
        
        // Expand redirection filenames
        redir = pipeline->commands[i]->redirections;
        while (redir)
        {
            // Don't expand filenames in heredocs (they're processed differently)
            if (redir->type != TOKEN_HEREDOC)
            {
                expanded = expand_variables(redir->file, shell);
                if (expanded)
                {
                    free(redir->file);
                    redir->file = expanded;
                }
            }
            redir = redir->next;
        }
    }
}
