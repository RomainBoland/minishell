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
char	*expand_variables(char *str, t_shell *shell)
{
	int	i;
	int	j;
	char	*result;
	char	*temp;
	char	*var_value;
	char	*quoted;

	i = 0;
	result = NULL;

	// Initialize result as empty string
	result = ft_strdup("");
	if (!result)
		return (NULL);
	
	// Process the string
	while (str[i])
	{
		if (str[i] == '$' && str[i + 1] != ' ' && str[i + 1] != '"' && str[i + 1] != '\'')
		{
			// Save start position
			j = i;

			// Expand the variable
			var_value = expand_single_var(str, &i, shell);

			// Append everything before the variable and the expanded value

			temp = result;
			result = ft_strjoin(result, var_value);
			free(temp);
			free(var_value);

			if (!result)
				return (NULL);
		}
		else if (str[i] == '\'' && !is_in_dquotes(str, i))
		{
			// Skip single quoted content (no expansion inside single quote)
			i++;
			j = i;
			while (str[i] && str[i] != '\'')
				i++;
			
			// Append the quoted content as is
			quoted = ft_substr(str, j, i - j);
			temp = result;
			result = ft_strjoin(result, quoted);
			free(temp);
			free(quoted);

			if (str[i])
				i++;
		}
		else
		{
			// Append regular character
			char c[2] = {str[i], '\0'};
			temp = result;
			result = ft_strjoin(result, c);
			free(temp);
			i++;
		}
	}

	return (result);
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
void	expand_command_args(t_command *cmd, t_shell *shell)
{
	int	i;
	char	*expanded;

	if (!cmd || !cmd->args)
		return ;

	i = 0;
	while  (cmd->args[i])
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

	// also expand redirection filenames
	if (cmd->input_file)
	{
		expanded = expand_variables(cmd->input_file, shell);
		if (expanded)
		{
			free(cmd->input_file);
			cmd->input_file = expanded;
		}
	}
	if (cmd->output_file)
	{
		expanded = expand_variables(cmd->output_file, shell);
		if (expanded)
		{
			free(cmd->output_file);
			cmd->output_file = expanded;
		}
	}
	if (cmd->heredoc_delim)
	{
		expanded = expand_variables(cmd->heredoc_delim, shell);
		if (expanded)
		{
			free(cmd->heredoc_delim);
			cmd->heredoc_delim = expanded;
		}
	}
}

// Expand variables in a pipeline of commands
void	expand_pipeline(t_pipeline *pipeline, t_shell *shell)
{
	int	i;

	i = 0;
	if (!pipeline)
		return ;

	while (i < pipeline->cmd_count)
	{
		expand_command_args(pipeline->commands[i], shell);
		i++;
	}
}
