/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <romain.boland@hotmail.com>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/19 11:14:41 by rboland           #+#    #+#             */
/*   Updated: 2025/03/26 09:50:53 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

// Echo command (with -n option support)
// TODO : echo -nnnnnnn / echo -nnnx 
int ft_echo(t_command *cmd)
{
    int i;
    int n_flag;
    
    if (!cmd || !cmd->args)
        return 1;
    
    // Check for -n option (check how many n / )
    i = 1;
    n_flag = 0;
    while (cmd->args[i] && ft_strncmp(cmd->args[i], "-n", 3) == 0)
    {
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
// TODO : cd - / cd ~
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
    
    // No arguments - go to HOME
    if (!cmd->args[1])
    {
        path = get_env_value(shell->env, "HOME");
        if (!path)
        {
            ft_putendl_fd("minishell: cd: HOME not set", STDERR_FILENO);
            return 1;
        }
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

// Export a variable to the environment
int ft_export(t_command *cmd, t_shell *shell)
{
    int i;
    char *equals_sign;
    
    if (!cmd || !cmd->args)
        return 1;
    
    // If no arguments, print current environment (sorted)
    if (!cmd->args[1])
    {
        return ft_env(shell);  // For simplicity, just call env for now
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
    
    return 0;
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
