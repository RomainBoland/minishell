/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <romain.boland@hotmail.com>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/19 11:14:41 by rboland           #+#    #+#             */
/*   Updated: 2025/04/10 13:27:23 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

// Echo command (with -n and -nnnn option support)
int ft_echo(t_command *cmd)
{
    int i;
    int j;
    int n_flag;
    
    if (!cmd || !cmd->args)
        return 1;
    
    // Check for -n option (check how many n / )
    i = 1;
    n_flag = 0;
    while (cmd->args[i] && cmd->args[i][0] == '-' && cmd->args[i][1] == 'n')
    {
        j = 1;
        while (cmd->args[i][j] == 'n')
            j++;
        if (cmd->args[i][j] != '\0')
            break;
        n_flag = 1;
        i++;  
    }

   // Print arguments with spaces between them
    while (cmd->args[i])
    {
        ft_putstr_fd(cmd->args[i], STDOUT_FILENO);
        if (cmd->args[i + 1])
            ft_putchar_fd(' ', STDOUT_FILENO);
        i++;
    }
    
    // Print newline unless -n flag is set
    if (!n_flag)
        ft_putchar_fd('\n', STDOUT_FILENO);
    
    return 0;
}

// Change directory command
// QUESTION : le vrai prompt est de bash: cd: -x: invalid option, faut il changer le message d'erreur duc?
int ft_cd(t_command *cmd, t_shell *shell)
{
    char *path;
    char old_pwd[PATH_MAX];
    
    if (!cmd || !cmd->args)
        return 1;
    
    // Save current directory
    if (getcwd(old_pwd, sizeof(old_pwd)) == NULL)
    {
        ft_putendl_fd("minishell: cd: error retrieving current directory", STDERR_FILENO);
        return 1;
    }

    // Two or more arguments - Error
    if (cmd->args[2])
    {
        ft_putendl_fd("minishell: cd: too many arguments", STDERR_FILENO);
        return (1);
    }

    // No arguments - go to HOME
    if (!cmd->args[1] 
        || ft_strncmp(cmd->args[1], "~", 2) == 0 
        || ft_strncmp(cmd->args[1], "~/", 3) == 0 
        || ft_strncmp(cmd->args[1], "--", 3) == 0)
    {
        path = get_env_value(shell->env, "HOME");
        if (!path)
        {
            ft_putendl_fd("minishell: cd: HOME not set", STDERR_FILENO);
            return (1);
        }
    }

    // Cd with path beginning by ~/
    else if (cmd->args[1][0] == '~' && cmd->args[1][1] == '/' && cmd->args[1][2] != '\0')
    {
        char *subpath = ft_substr(cmd->args[1], 1, ft_strlen(cmd->args[1]) - 1);
        path = ft_strjoin(get_env_value(shell->env, "HOME"), subpath);
        if (!path)
        {
            ft_putendl_fd("minishell: cd: HOME not set", STDERR_FILENO);
            return (1);
        }
    }
    // Cd with - 
    else if (ft_strncmp(cmd->args[1], "-", 2) == 0)
    {
        path = get_env_value(shell->env, "OLDPWD");
        printf("%s\n", path);
        if (!path)
        {
            ft_putendl_fd("minishell: cd: OLDPWD not set", STDERR_FILENO);
            return (1);
        }
    }
    // Cd with wrong option
    else if (cmd->args[1][0] == '-' && cmd->args[1][1] != '\0')
    {
        ft_putendl_fd("minishell: cd: invalid option", STDERR_FILENO);
        return (2);
    }
    else
    {
        path = cmd->args[1];
    }

    // Change directory
    if (chdir(path) != 0)
    {
        ft_putstr_fd("minishell: cd: ", STDERR_FILENO);
        ft_putstr_fd(path, STDERR_FILENO);
        ft_putstr_fd(": ", STDERR_FILENO);
        ft_putendl_fd(strerror(errno), STDERR_FILENO);
        return 1;
    }
    
    // Update PWD and OLDPWD in environment
    set_env_value(shell->env, "OLDPWD", old_pwd);
    
    // Get new PWD
    char new_pwd[PATH_MAX];
    if (getcwd(new_pwd, sizeof(new_pwd)) != NULL)
    {
        set_env_value(shell->env, "PWD", new_pwd);
    }
    
    return 0;
}

// Print working directory command
int ft_pwd(void)
{
    char cwd[PATH_MAX];
    
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        ft_putendl_fd("minishell: pwd: error retrieving current directory", STDERR_FILENO);
        return 1;
    }
    
    ft_putendl_fd(cwd, STDOUT_FILENO);
    return 0;
}


// Sert a trier l'array d'environnement
void    bubble_sort(char **env_array)
{
    int     len;
    int     i;
    int     j;
    char    *temp;

    len = 0;
    while (env_array[len])
        len++;
    i = 0;
    while (i < len - 1)
    {
        j = 0;
        while (j < len - 1)
        {
            if (ft_strcmp(env_array[j], env_array[j + 1]) > 0)
            {
                temp = env_array[j];
                env_array[j] = env_array[j + 1];
                env_array[j + 1] = temp;
            }
            j++;
        }
        i++;
    }
}


// Export a variable to the environment
// _= apparait dans l'export et l'env alors qu'il n'apparait pas dans l'export normalement
// _= ne prend pas la bonne valeur, il reste bloque a ./minishell
// faire en sorte que export x donne dans export x et pas x=""
int ft_export(t_command *cmd, t_shell *shell)
{
    int i;
    char *equals_sign;
    char    **env_array;
    
    if (!cmd || !cmd->args)
        return 1;
    env_array = NULL;
    i = 0;
    // If no arguments, print current environment (sorted)
    if (!cmd->args[1])
    {
        env_array = env_to_array(shell->env);
        bubble_sort(env_array);
        

        while (env_array[i])
        {
            printf("%s\n", env_array[i]);
            // equals_sign = ft_strchr(env_array[i], '=');
            // ft_putstr_fd("declare -x ", STDOUT_FILENO);
            // if (equals_sign)
            // {
            //     write(1, env_array[i], equals_sign - env_array[i]);
            //     ft_putstr_fd("=\"", STDOUT_FILENO);
            //     ft_putstr_fd(equals_sign + 1, STDOUT_FILENO);
            //     ft_putstr_fd("\"\n", STDOUT_FILENO);
            // }
            // else
            // {
            //     ft_putstr_fd(env_array[i], STDOUT_FILENO);
            //     ft_putchar_fd('\n', STDOUT_FILENO);
            // }
            i++;        
        }
    return (0);
    }
    
    // Process each argument
    for (i = 1; cmd->args[i]; i++)
    {
        // Find equals sign
        equals_sign = ft_strchr(cmd->args[i], '=');
        
        if (equals_sign)
        {
            // Split into name and value
            *equals_sign = '\0';
            set_env_value(shell->env, cmd->args[i], equals_sign + 1);
            *equals_sign = '=';
        }
        else
        {
            // If no equals sign, just add to environment with empty value if it doesn't exist
            char *value = get_env_value(shell->env, cmd->args[i]);
            if (!value)
                set_env_value(shell->env, cmd->args[i], "");
        }
    }
    
    return 0;
}

// Remove a variable from the environment
int ft_unset(t_command *cmd, t_shell *shell)
{
    int i;
    
    if (!cmd || !cmd->args)
        return 1;
    
    // Process each argument
    for (i = 1; cmd->args[i]; i++)
    {
        remove_env_var(shell->env, cmd->args[i]);
    }
    
    return 0;
}

// Print the environment
int ft_env(t_shell *shell)
{
    t_env *current = shell->env;
    
    while (current)
    {
        // Only print variables that have a value
        if (current->value && *current->value)
        {
            ft_putstr_fd(current->key, STDOUT_FILENO);
            ft_putchar_fd('=', STDOUT_FILENO);
            ft_putendl_fd(current->value, STDOUT_FILENO);
        }
        current = current->next;
    }
    
    return (0);
}

// Exit the shell
int ft_exit(t_command *cmd, t_shell *shell)
{
    int exit_code = 0;
    
    if (!cmd || !cmd->args)
        return 1;
    
    ft_putendl_fd("exit", STDOUT_FILENO);
    
    // If there's an argument, use it as exit code
    if (cmd->args[1])
    {
        // TODO: Add proper numeric validation
        exit_code = ft_atoi(cmd->args[1]);
    }
    
    // Free environment variables
    free_env(shell->env);
    
    // Clear readline history
    rl_clear_history();
    
    exit(exit_code);
    return exit_code;  // This won't execute, but keeps the compiler happy
}
