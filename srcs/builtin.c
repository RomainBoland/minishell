/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <romain.boland@hotmail.com>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/19 11:14:41 by rboland           #+#    #+#             */
/*   Updated: 2025/04/14 10:55:02 by rboland          ###   ########.fr       */
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
    char *pwd_env;
    
    if (!cmd || !cmd->args)
        return 1;
    
    // Try to get current directory, but don't fail immediately if it's not available
    if (getcwd(old_pwd, sizeof(old_pwd)) == NULL)
    {
        // Instead of failing, try to get PWD from environment
        ft_putendl_fd("cd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory", STDERR_FILENO);
        pwd_env = get_env_value(shell->env, "PWD");
        if (pwd_env)
            ft_strlcpy(old_pwd, pwd_env, PATH_MAX);
        else
        {
            ft_putendl_fd("minishell: cd: error retrieving current directory", STDERR_FILENO);
            return 1;
        }
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
        free(subpath);
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
    
    // Update OLDPWD in environment
    set_env_value(shell->env, "OLDPWD", old_pwd);
    
    // Try to get new PWD
    char new_pwd[PATH_MAX];
    if (getcwd(new_pwd, sizeof(new_pwd)) != NULL)
    {
        set_env_value(shell->env, "PWD", new_pwd);
    }
    else
    {
        // If getcwd fails (likely deleted directory), compute logical path
        char *new_path;
        
        if (path[0] == '/')
        {
            // Absolute path
            new_path = ft_strdup(path);
        }
        else
        {
            // Relative path, need to compute
            if (ft_strcmp(path, "..") == 0)
            {
                // Go up one directory from PWD
                char *last_slash = ft_strrchr(old_pwd, '/');
                if (last_slash == old_pwd)
                    new_path = ft_strdup("/"); // Root directory
                else
                    new_path = ft_substr(old_pwd, 0, last_slash - old_pwd);
            }
            else if (ft_strcmp(path, ".") == 0)
            {
                // Stay in same directory
                new_path = ft_strdup(old_pwd);
            }
            else
            {
                // Append the path to old_pwd
                if (old_pwd[ft_strlen(old_pwd) - 1] == '/')
                    new_path = ft_strjoin(old_pwd, path);
                else
                {
                    char *temp = ft_strjoin(old_pwd, "/");
                    new_path = ft_strjoin(temp, path);
                    free(temp);
                }
            }
        }
        
        if (new_path)
        {
            set_env_value(shell->env, "PWD", new_path);
            free(new_path);
        }
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

int is_argument_valid(char *str)
{
    int i = 0;

    if (!ft_isalpha(str[0]) && str[0] != '_')
    {
        printf("minishell: export: `%s': not a valid identifier\n", str);
        return (0);
    }
    while (str[i] && str[i] != '=')
    {
        if (!ft_isalnum(str[i]) && str[i] != '_' && str[i] != '+' )
        {
            printf("minishell: export: `%s': not a valid identifier\n", str);
            return (0);
        }
        i++;
    }
    return (1);
}


int write_export(t_command *cmd)
{
    int i;

    i = 1;
    while (cmd->args[i])
    {
        if (cmd->args[i][0] != '\0')
            return (0);
        i++;
    }
    return (1);
}

// Export a variable to the environment
// Faire export $oui $non $etc
int ft_export(t_command *cmd, t_shell *shell)
{
    int i;
    int error_flag = 0;
    char *equals_sign;
    char    **env_array;
    
    if (!cmd || !cmd->args)
        return 1;
    env_array = NULL;
    i = 0;
    // If no arguments, print current environment (sorted)
    // If only empty arguments, print current environment (sorted)
    // A FAIRE
    if (!cmd->args[1] || write_export(cmd))
    {
        env_array = env_to_array_export(shell->env);
        bubble_sort(env_array);

        while (env_array[i])
        {
            printf("declare -x %s\n", env_array[i]);
            free(env_array[i]);
            i++;
        }
        free(env_array);
        return (0);
    }
    
    // Process each argument
    for (i = 1; cmd->args[i]; i++)
    {
        if (cmd->args[i][0] == '\0')
            continue;
        // Fonction qui permet de verifier que l'argument est valide
        if (!is_argument_valid(cmd->args[i]))
        {
            error_flag = 1;
            continue;
        }
        char *plus_equal = ft_strnstr(cmd->args[i], "+=", ft_strlen(cmd->args[i]));
        if (plus_equal)
        {
            // Ajout de valeur à une variable déjà existante
            *plus_equal = '\0'; // Séparer la clé
            char *key = cmd->args[i];
            char *to_append = plus_equal + 2;
            char *existing = get_env_value(shell->env, key);
            char *new_val = ft_strjoin(existing ? existing : "", to_append);
            set_env_value(shell->env, key, new_val);
            free(new_val);
            continue;
        }

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
            // If no equals sign, just add to environment with NULL value if it doesn't exist
            char *value = get_env_value(shell->env, cmd->args[i]);
            if (!value && !has_env_key(shell->env, cmd->args[i]))
                set_env_value(shell->env, cmd->args[i], NULL);
        }
    }
    
    return (error_flag);
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
// Probleme pour x= qui devrait apparaitre dans l'envi
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
        // Check for numeric argument
        char *arg = cmd->args[1];
        int i = 0;
        int is_numeric = 1;
        
        // Check if numeric (allow one leading +/-)
        if (arg[0] == '+' || arg[0] == '-')
            i++;
            
        while (arg[i])
        {
            if (!ft_isdigit(arg[i]))
            {
                is_numeric = 0;
                break;
            }
            i++;
        }
        
        if (!is_numeric)
        {
            ft_putstr_fd("minishell: exit: ", STDERR_FILENO);
            ft_putstr_fd(arg, STDERR_FILENO);
            ft_putendl_fd(": numeric argument required", STDERR_FILENO);
            exit_code = 2;
        }
        else if (cmd->args[2])
        {
            ft_putendl_fd("minishell: exit: too many arguments", STDERR_FILENO);
            return 1;
        }
        else
        {
            exit_code = ft_atoi(arg);
        }
    }
    
    // Free environment variables
    free_env(shell->env);
    
    // Clear readline history
    rl_clear_history();
    
    exit(exit_code);
    return exit_code;  // This won't execute, but keeps the compiler happy
}
