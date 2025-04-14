/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/24 15:13:42 by rboland           #+#    #+#             */
/*   Updated: 2025/03/24 15:13:42 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

int	ft_strcmp(char *s1, char *s2)
{
	int	i;

	i = 0;
	while (s1[i] && s2[i])
	{
		if (s1[i] > s2[i] || s1[i] < s2[i])
		{
			return (s1[i] - s2[i]);
		}
		i++;
	}
	return (s1[i] - s2[i]);
}

int has_unclosed_quotes(char *input)
{
    int i = 0;
    int in_single_quotes = 0;
    int in_double_quotes = 0;
    
    while (input[i])
    {
        if (input[i] == '\'' && !in_double_quotes)
            in_single_quotes = !in_single_quotes;
        else if (input[i] == '\"' && !in_single_quotes)
            in_double_quotes = !in_double_quotes;
        i++;
    }
    
    return (in_single_quotes || in_double_quotes);
}
