/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <romain.boland@hotmail.com>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/19 11:05:20 by rboland           #+#    #+#             */
/*   Updated: 2025/03/19 12:55:50 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

// Check if command is a built-in
int is_builtin(char *cmd)
{
    if (!cmd)
        return 0;
    
    if (ft_strncmp(cmd, "echo", 5) == 0 ||
        ft_strncmp(cmd, "cd", 3) == 0 ||
        ft_strncmp(cmd, "pwd", 4) == 0 ||
        ft_strncmp(cmd, "export", 7) == 0 ||
        ft_strncmp(cmd, "unset", 6) == 0 ||
        ft_strncmp(cmd, "env", 4) == 0 ||
        ft_strncmp(cmd, "exit", 5) == 0)
        return 1;
    
    return 0;
}

// Execute a built-in command
int execute_builtin(t_command *cmd, t_shell *shell)
{
    if (!cmd || !cmd->args || !cmd->args[0])
        return 1;
    
    if (ft_strncmp(cmd->args[0], "echo", 5) == 0)
        return ft_echo(cmd);
    else if (ft_strncmp(cmd->args[0], "cd", 3) == 0)
        return ft_cd(cmd, shell);
    else if (ft_strncmp(cmd->args[0], "pwd", 4) == 0)
        return ft_pwd();
    else if (ft_strncmp(cmd->args[0], "export", 7) == 0)
        return ft_export(cmd, shell);
    else if (ft_strncmp(cmd->args[0], "unset", 6) == 0)
        return ft_unset(cmd, shell);
    else if (ft_strncmp(cmd->args[0], "env", 4) == 0)
        return ft_env(shell);
    else if (ft_strncmp(cmd->args[0], "exit", 5) == 0)
        return ft_exit(cmd, shell);
    
    return 1;
}

// Find executable in PATH
char *find_executable(char *cmd, t_shell *shell)
{
    char *path;
    char **paths;
    char *exec_path;
    int i;
    
    // Return the command if it's an absolute or relative path
    if (ft_strchr(cmd, '/'))
    {
        if (access(cmd, X_OK) == 0)
            return ft_strdup(cmd);
        return NULL;
    }
    
    // Get PATH environment variable
    path = get_env_value(shell->env, "PATH");
    if (!path)
        return NULL;
    
    // Split PATH into directories
    paths = ft_split(path, ':');
    if (!paths)
        return NULL;
    
    // Search for executable in each directory
    i = 0;
    exec_path = NULL;
    while (paths[i])
    {
        char *dir_with_slash = ft_strjoin(paths[i], "/");
        char *full_path = ft_strjoin(dir_with_slash, cmd);
        free(dir_with_slash);
        
        if (access(full_path, X_OK) == 0)
        {
            exec_path = full_path;
            break;
        }
        
        free(full_path);
        i++;
    }
    
    // Free paths array
    i = 0;
    while (paths[i])
        free(paths[i++]);
    free(paths);
    
    return exec_path;
}

// Set up redirections for a command
int setup_redirections(t_command *cmd, int in_fd, int out_fd)
{
    int fd;
    
    // Set up input redirection
    if (cmd->input_file)
    {
        fd = open(cmd->input_file, O_RDONLY);
        if (fd < 0)
        {
            perror(cmd->input_file);
            return 0;
        }
        
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    else if (in_fd != STDIN_FILENO)
    {
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    }
    
    // Set up output redirection
    if (cmd->output_file)
    {
        int flags = O_WRONLY | O_CREAT;
        if (cmd->append_output)
            flags |= O_APPEND;
        else
            flags |= O_TRUNC;
        
        fd = open(cmd->output_file, flags, 0644);
        if (fd < 0)
        {
            perror(cmd->output_file);
            return 0;
        }
        
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    else if (out_fd != STDOUT_FILENO)
    {
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }
    
    return 1;
}

// Execute a single command with redirections
int execute_command(t_command *cmd, int in_fd, int out_fd, t_shell *shell)
{
    pid_t pid;
    int status;
    char *exec_path;
    
    // Handle empty command
    if (!cmd || !cmd->args || !cmd->args[0])
        return 0;
    
    // Check if it's a built-in command
    if (is_builtin(cmd->args[0]))
    {
        // Save original stdin/stdout
        int stdin_copy = dup(STDIN_FILENO);
        int stdout_copy = dup(STDOUT_FILENO);
        
        // Set up redirections
        if (!setup_redirections(cmd, in_fd, out_fd))
        {
            // Restore stdin/stdout
            dup2(stdin_copy, STDIN_FILENO);
            dup2(stdout_copy, STDOUT_FILENO);
            close(stdin_copy);
            close(stdout_copy);
            return 1;
        }
        
        // Execute built-in
        status = execute_builtin(cmd, shell);
        
        // Restore stdin/stdout
        dup2(stdin_copy, STDIN_FILENO);
        dup2(stdout_copy, STDOUT_FILENO);
        close(stdin_copy);
        close(stdout_copy);
        
        return status;
    }
    
    // Find executable path
    exec_path = find_executable(cmd->args[0], shell);
    if (!exec_path)
    {
        ft_putstr_fd("minishell: command not found: ", 2);
        ft_putendl_fd(cmd->args[0], 2);
        return 127;  // Command not found exit status
    }
    
    // Fork and execute
    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        free(exec_path);
        return 1;
    }
    
    if (pid == 0)  // Child process
    {
        // Set up signal handlers for child
        signal(SIGINT, SIG_DFL);    // Default Ctrl+C behavior
        signal(SIGQUIT, SIG_DFL);   // Default Ctrl+\ behavior
        
        // Set up redirections
        if (!setup_redirections(cmd, in_fd, out_fd))
            exit(1);
        
        // Close unneeded file descriptors
        if (in_fd != STDIN_FILENO)
            close(in_fd);
        if (out_fd != STDOUT_FILENO)
            close(out_fd);
        
        // Convert environment to array for execve
        char **env_array = env_to_array(shell->env);
        if (!env_array)
            exit(1);
        
        // Execute command
        execve(exec_path, cmd->args, env_array);
        
        // If execve returns, there was an error
        perror(exec_path);
        free(exec_path);
        free_env_array(env_array);
        exit(1);
    }
    
    // Parent process
    free(exec_path);
    
    // Close pipe file descriptors in parent
    if (in_fd != STDIN_FILENO)
        close(in_fd);
    if (out_fd != STDOUT_FILENO)
        close(out_fd);
    
    // Wait for child to finish
    waitpid(pid, &status, 0);
    
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    
    return 1;
}

// Execute a pipeline of commands
int execute_pipeline(t_pipeline *pipeline, t_shell *shell)
{
    int i;
    int in_fd;
    int pipefds[2];
    int exit_status = 0;
    
    if (!pipeline || pipeline->cmd_count == 0)
        return 1;
    
    // Start with standard input
    in_fd = STDIN_FILENO;
    
    // Process all commands in the pipeline
    for (i = 0; i < pipeline->cmd_count; i++)
    {
        // Last command
        if (i == pipeline->cmd_count - 1)
        {
            // Execute with current input and standard output
            exit_status = execute_command(pipeline->commands[i], in_fd, STDOUT_FILENO, shell);
        }
        else
        {
            // Create pipe for the next command
            if (pipe(pipefds) < 0)
            {
                perror("pipe");
                return 1;
            }
            
            // Execute with current input and pipe output
            exit_status = execute_command(pipeline->commands[i], in_fd, pipefds[1], shell);
            
            // Close write end of pipe
            close(pipefds[1]);
            
            // Update input for next command
            if (in_fd != STDIN_FILENO)
                close(in_fd);
            in_fd = pipefds[0];
        }
    }
    
    return exit_status;
}
