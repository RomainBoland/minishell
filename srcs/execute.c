/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <romain.boland@hotmail.com>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/19 11:05:20 by rboland           #+#    #+#             */
/*   Updated: 2025/04/24 09:52:00 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

int setup_heredoc(t_command *cmd, t_shell *shell)
{
    int     pipe_fd[2];
    char    *line;
    pid_t   pid;
    int     status;
    struct sigaction old_int, old_quit, sa_int, sa_quit;
    
    if (!cmd->has_heredoc || cmd->heredoc_count == 0)
        return STDIN_FILENO;
    
    if (pipe(pipe_fd) < 0)
    {
        perror("pipe");
        return -1;
    }
    
    // Save current signal handlers
    sigaction(SIGINT, NULL, &old_int);
    sigaction(SIGQUIT, NULL, &old_quit);
    
    // Set up temporary signal handlers for parent process during heredoc
    ft_memset(&sa_int, 0, sizeof(sa_int));
    ft_memset(&sa_quit, 0, sizeof(sa_quit));
    sa_int.sa_handler = SIG_IGN;  // Ignore SIGINT in parent
    sa_quit.sa_handler = SIG_IGN; // Ignore SIGQUIT in parent
    sigaction(SIGINT, &sa_int, NULL);
    sigaction(SIGQUIT, &sa_quit, NULL);
    
    // Fork to handle signals properly in heredoc
    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        
        // Restore original signal handlers
        sigaction(SIGINT, &old_int, NULL);
        sigaction(SIGQUIT, &old_quit, NULL);
        return -1;
    }
    
    if (pid == 0) // Child process
    {
        // Set up signal handling for child (heredoc reader)
        struct sigaction sa_child_int;
        ft_memset(&sa_child_int, 0, sizeof(sa_child_int));
        sa_child_int.sa_handler = SIG_DFL;  // Default behavior for Ctrl+C
        sigaction(SIGINT, &sa_child_int, NULL);
        
        close(pipe_fd[0]);  // Close read end in child
        
        // Process each heredoc delimiter in order
        for (int i = 0; i < cmd->heredoc_count; i++)
        {
            // For each heredoc, we collect its input lines
            char *collected_input = ft_strdup("");
            
            // Read lines until current delimiter is found
            while (1)
            {
                line = readline("> ");  // Prompt for heredoc input
                
                // If line is NULL (Ctrl+D) or matches delimiter, break
                if (!line || ft_strcmp(line, cmd->heredoc_delims[i]) == 0)
                {
                    free(line);
                    break;
                }
                
                // Only for the last heredoc, save the content
                if (i == cmd->heredoc_count - 1)
                {
                    // Check if we should expand variables (only if delimiter wasn't quoted)
                    if (!cmd->heredoc_quoted[i])
                    {
                        // Expand variables
                        char *expanded = expand_variables(line, shell);
                        if (expanded)
                        {
                            char *temp = collected_input;
                            collected_input = ft_strjoin(collected_input, expanded);
                            free(temp);
                            temp = collected_input;
                            collected_input = ft_strjoin(collected_input, "\n");
                            free(temp);
                            free(expanded);
                        }
                        else
                        {
                            char *temp = collected_input;
                            collected_input = ft_strjoin(collected_input, line);
                            free(temp);
                            temp = collected_input;
                            collected_input = ft_strjoin(collected_input, "\n");
                            free(temp);
                        }
                    }
                    else
                    {
                        // Delimiter was quoted, don't expand variables
                        char *temp = collected_input;
                        collected_input = ft_strjoin(collected_input, line);
                        free(temp);
                        temp = collected_input;
                        collected_input = ft_strjoin(collected_input, "\n");
                        free(temp);
                    }
                }
                
                free(line);
            }
            
            // If this is the last heredoc, write its content to the pipe
            if (i == cmd->heredoc_count - 1)
            {
                write(pipe_fd[1], collected_input, ft_strlen(collected_input));
            }
            
            free(collected_input);
        }
        
        close(pipe_fd[1]);
        exit(0);
    }
    else // Parent process
    {
        close(pipe_fd[1]);  // Close write end in parent
        
        // Wait for child to complete
        waitpid(pid, &status, 0);
        
        // Restore original signal handlers
        sigaction(SIGINT, &old_int, NULL);
        sigaction(SIGQUIT, &old_quit, NULL);
        
        // Check if child was terminated by a signal
        if (WIFSIGNALED(status))
        {
            // If child was terminated by SIGINT (Ctrl+C)
            if (WTERMSIG(status) == SIGINT)
            {
                // Let ^C display on its own without adding newline
                // Make sure readline is in a good state
                rl_on_new_line();
            }
            close(pipe_fd[0]);
            return -1;  // Indicate heredoc was interrupted
        }
    }
    
    return pipe_fd[0];  // Return read end of pipe
}

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
    t_redirection *redir;
    int fd;
    
    if (!cmd)
        return 0;
    
    // Set default stdin/stdout if no custom ones provided
    if (in_fd != STDIN_FILENO)
    {
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    }
    
    if (out_fd != STDOUT_FILENO)
    {
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }
    
    // Process all redirections in order
    redir = cmd->redirections;
    while (redir)
    {
        // Handle input redirection (<)
        if (redir->type == TOKEN_REDIR_IN)
        {
            fd = open(redir->file, O_RDONLY);
            if (fd < 0)
            {
                perror(redir->file);
                return 0;
            }
            
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        // Handle output redirection (>)
        else if (redir->type == TOKEN_REDIR_OUT)
        {
            fd = open(redir->file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0)
            {
                perror(redir->file);
                return 0;
            }
            
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        // Handle append output redirection (>>)
        else if (redir->type == TOKEN_APPEND)
        {
            fd = open(redir->file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0)
            {
                perror(redir->file);
                return 0;
            }
            
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        // Heredoc is handled separately
        
        redir = redir->next;
    }
    
    return 1;
}

int execute_command(t_command *cmd, int in_fd, int out_fd, t_shell *shell)
{
    int status;
    int heredoc_fd = STDIN_FILENO;
    pid_t pid;
    
    // Handle empty command
    if (!cmd || !cmd->args || !cmd->args[0])
        return 0;
    
    // Set up heredoc if needed
    if (cmd->has_heredoc)
    {
        heredoc_fd = setup_heredoc(cmd, shell);
        if (heredoc_fd < 0)
            return 1;  // Heredoc setup failed or was interrupted
        
        int has_input_redirection = 0;
        t_redirection *redir = cmd->redirections;
        while (redir)
        {
            if (redir->type == TOKEN_REDIR_IN)
            {
                has_input_redirection = 1;
                break;
            }
            redir = redir->next;
        }
        if (!has_input_redirection)
            in_fd = heredoc_fd;
    }
    
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
            if (heredoc_fd != STDIN_FILENO)
                close(heredoc_fd);
            return 1;
        }
        
        // Execute built-in
        status = execute_builtin(cmd, shell);
        
        // Restore stdin/stdout
        dup2(stdin_copy, STDIN_FILENO);
        dup2(stdout_copy, STDOUT_FILENO);
        close(stdin_copy);
        close(stdout_copy);
        
        if (heredoc_fd != STDIN_FILENO)
            close(heredoc_fd);
        
        return status;
    }
    else
    {
        // Find executable path
        char *exec_path = find_executable(cmd->args[0], shell);
        if (!exec_path)
        {
            ft_putstr_fd("minishell: command not found: ", 2);
            ft_putendl_fd(cmd->args[0], 2);
            if (heredoc_fd != STDIN_FILENO)
                close(heredoc_fd);
            return 127;  // Command not found exit status
        }
        
        // Fork and execute
        pid = fork();
        if (pid < 0)
        {
            perror("fork");
            free(exec_path);
            if (heredoc_fd != STDIN_FILENO)
                close(heredoc_fd);
            return 1;
        }
        
        if (pid == 0)  // Child process
        {
            // Special handling for nested minishell
            if (cmd->args[0] && ft_strcmp(cmd->args[0], "./minishell") == 0)
            {
                // For nested minishell, we need special signal handling
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
                
                // Create a new process group for the nested shell
                // This helps with signal handling
                setpgid(0, 0);
            }
            else
            {
                // Normal signal handlers for other commands
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGTTIN, SIG_DFL);
                signal(SIGTTOU, SIG_DFL);
                signal(SIGCHLD, SIG_DFL);
            }
            
            // Set up redirections
            if (!setup_redirections(cmd, in_fd, out_fd))
            {
                free(exec_path);
                exit(1);
            }
            
            // Close unneeded file descriptors
            if (in_fd != STDIN_FILENO)
                close(in_fd);
            if (out_fd != STDOUT_FILENO)
                close(out_fd);
            if (heredoc_fd != STDIN_FILENO && heredoc_fd != in_fd)
                close(heredoc_fd);
            
            // Convert environment to array for execve
            char **env_array = env_to_array(shell->env);
            if (!env_array)
            {
                free(exec_path);
                exit(1);
            }
            
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
        
        // Special handling for parent of nested minishell
        if (cmd->args[0] && ft_strcmp(cmd->args[0], "./minishell") == 0)
        {
            // For nested minishell, make it the foreground process group
            // This ensures signals sent to the terminal go to the nested shell
            setpgid(pid, pid);
            tcsetpgrp(STDIN_FILENO, pid);
        }
        
        // Close heredoc fd in parent since it's duplicated in child
        if (heredoc_fd != STDIN_FILENO)
            close(heredoc_fd);
        
        // Wait for child to finish
        waitpid(pid, &status, 0);
        
        // If we gave terminal control to a nested minishell, take it back
        if (cmd->args[0] && ft_strcmp(cmd->args[0], "./minishell") == 0)
        {
            tcsetpgrp(STDIN_FILENO, getpgrp());
        }
        
        if (WIFEXITED(status))
            return WEXITSTATUS(status);
        else if (WIFSIGNALED(status))
            return 128 + WTERMSIG(status);
        
        return 1;
    }
}

// Execute a pipeline of commands
// Modified version of execute_pipeline in execute.c
int execute_pipeline(t_pipeline *pipeline, t_shell *shell)
{
    int i;
    int in_fd;
    int pipefds[2][2];  // Two sets of pipe file descriptors for alternating
    int active_pipe = 0;  // Track which pipe set is active
    int exit_status = 0;
    pid_t *pids;  // Array to store all process IDs
    int *heredoc_fds; // Array to store heredoc file descriptors
    
    if (!pipeline || pipeline->cmd_count == 0)
        return 1;
    
    // Special case: Single built-in command without pipes
    if (pipeline->cmd_count == 1 && pipeline->commands[0]->args && 
        is_builtin(pipeline->commands[0]->args[0]))
    {
        // For built-ins, execute directly in the current process
        return execute_command(pipeline->commands[0], STDIN_FILENO, STDOUT_FILENO, shell);
    }
    
    // Allocate memory for process IDs and heredoc fds
    pids = malloc(sizeof(pid_t) * pipeline->cmd_count);
    heredoc_fds = malloc(sizeof(int) * pipeline->cmd_count);
    if (!pids || !heredoc_fds)
    {
        free(pids);
        free(heredoc_fds);
        return 1;
    }
    
    // Set up heredocs for all commands first
    for (i = 0; i < pipeline->cmd_count; i++)
    {
        if (pipeline->commands[i]->has_heredoc)
        {
            heredoc_fds[i] = setup_heredoc(pipeline->commands[i], shell);
            if (heredoc_fds[i] < 0)
            {
                // Cleanup already created heredocs
                while (--i >= 0)
                {
                    if (heredoc_fds[i] != STDIN_FILENO)
                        close(heredoc_fds[i]);
                }
                free(pids);
                free(heredoc_fds);
                return 1;
            }
        }
        else
        {
            heredoc_fds[i] = STDIN_FILENO; // Default
        }
    }
    
    // Create initial pipes if needed
    if (pipeline->cmd_count > 1)
    {
        if (pipe(pipefds[active_pipe]) < 0)
        {
            perror("pipe");
            for (i = 0; i < pipeline->cmd_count; i++)
            {
                if (heredoc_fds[i] != STDIN_FILENO)
                    close(heredoc_fds[i]);
            }
            free(pids);
            free(heredoc_fds);
            return 1;
        }
    }
    
    // Process all commands in the pipeline
    for (i = 0; i < pipeline->cmd_count; i++)
    {
        int out_fd;
        
        // Determine output file descriptor
        if (i == pipeline->cmd_count - 1)
        {
            // Last command outputs to stdout
            out_fd = STDOUT_FILENO;
        }
        else
        {
            // Not the last command, output to pipe
            out_fd = pipefds[active_pipe][1];
            
            // Create pipe for the next command if not the second-to-last command
            if (i < pipeline->cmd_count - 2)
            {
                if (pipe(pipefds[1 - active_pipe]) < 0)
                {
                    perror("pipe");
                    // Clean up properly
                    for (int j = 0; j < pipeline->cmd_count; j++)
                    {
                        if (heredoc_fds[j] != STDIN_FILENO)
                            close(heredoc_fds[j]);
                    }
                    
                    // Close created pipes
                    if (i > 0)
                        close(in_fd);
                    close(pipefds[active_pipe][0]);
                    close(pipefds[active_pipe][1]);
                    
                    // Wait for any already started processes
                    for (int j = 0; j < i; j++)
                    {
                        if (pids[j] > 0)
                            waitpid(pids[j], NULL, 0);
                    }
                    
                    free(pids);
                    free(heredoc_fds);
                    return 1;
                }
            }
        }
        
        // Execute the command
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            // Similar cleanup as above
            for (int j = 0; j < pipeline->cmd_count; j++)
            {
                if (heredoc_fds[j] != STDIN_FILENO)
                    close(heredoc_fds[j]);
            }
            
            if (i > 0)
                close(in_fd);
            
            if (i < pipeline->cmd_count - 1)
            {
                close(pipefds[active_pipe][0]);
                close(pipefds[active_pipe][1]);
                
                if (i < pipeline->cmd_count - 2)
                {
                    close(pipefds[1 - active_pipe][0]);
                    close(pipefds[1 - active_pipe][1]);
                }
            }
            
            // Wait for any already started processes
            for (int j = 0; j < i; j++)
            {
                if (pids[j] > 0)
                    waitpid(pids[j], NULL, 0);
            }
            
            free(pids);
            free(heredoc_fds);
            return 1;
        }
        
        if (pid == 0)  // Child process
        {
            // Set up signal handlers for child
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);
            
            // Child doesn't need to keep track of pids
            free(pids);
            
            // IMPORTANT: Each command should handle its own redirections independently
            // Check for input redirection first
            int input_from_redir = 0;
            t_redirection *redir = pipeline->commands[i]->redirections;
            while (redir)
            {
                if (redir->type == TOKEN_REDIR_IN)
                {
                    int fd = open(redir->file, O_RDONLY);
                    if (fd < 0)
                    {
                        // Report error but IMPORTANT: exit with proper status
                        perror(redir->file);
                        
                        // Clean up before exiting
                        for (int j = 0; j < pipeline->cmd_count; j++)
                            if (heredoc_fds[j] != STDIN_FILENO)
                                close(heredoc_fds[j]);
                        free(heredoc_fds);
                        
                        if (i > 0 && in_fd != STDIN_FILENO)
                            close(in_fd);
                            
                        if (i < pipeline->cmd_count - 1)
                        {
                            close(pipefds[active_pipe][0]);
                            close(pipefds[active_pipe][1]);
                            
                            if (i < pipeline->cmd_count - 2)
                            {
                                close(pipefds[1 - active_pipe][0]);
                                close(pipefds[1 - active_pipe][1]);
                            }
                        }
                        
                        exit(1); // Exit with error status
                    }
                    
                    // Successfully opened input redirection
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                    input_from_redir = 1;
                }
                redir = redir->next;
            }
            
            // If no input redirection, use pipe or heredoc
            if (!input_from_redir)
            {
                if (i > 0) // Not the first command
                {
                    dup2(in_fd, STDIN_FILENO);
                    close(in_fd);
                }
                else if (heredoc_fds[i] != STDIN_FILENO) // First command with heredoc
                {
                    dup2(heredoc_fds[i], STDIN_FILENO);
                }
            }
            
            // Close all heredoc FDs in child
            for (int j = 0; j < pipeline->cmd_count; j++)
            {
                if (heredoc_fds[j] != STDIN_FILENO)
                    close(heredoc_fds[j]);
            }
            free(heredoc_fds);
            
            // Handle output redirections (including pipes)
            int output_redirected = 0;
            redir = pipeline->commands[i]->redirections;
            while (redir)
            {
                if (redir->type == TOKEN_REDIR_OUT)
                {
                    int fd = open(redir->file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0)
                    {
                        perror(redir->file);
                        exit(1);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                    output_redirected = 1;
                }
                else if (redir->type == TOKEN_APPEND)
                {
                    int fd = open(redir->file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (fd < 0)
                    {
                        perror(redir->file);
                        exit(1);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                    output_redirected = 1;
                }
                redir = redir->next;
            }
            
            // If no output redirection but not last command, connect to pipe
            if (!output_redirected && i < pipeline->cmd_count - 1)
            {
                dup2(out_fd, STDOUT_FILENO);
            }
            
            // Close all pipe file descriptors
            if (i < pipeline->cmd_count - 1)
            {
                close(pipefds[active_pipe][0]);
                close(pipefds[active_pipe][1]);
                
                if (i < pipeline->cmd_count - 2)
                {
                    close(pipefds[1 - active_pipe][0]);
                    close(pipefds[1 - active_pipe][1]);
                }
            }
            
            // Execute command
            if (is_builtin(pipeline->commands[i]->args[0]))
            {
                exit(execute_builtin(pipeline->commands[i], shell));
            }
            else
            {
                char *exec_path = find_executable(pipeline->commands[i]->args[0], shell);
                if (!exec_path)
                {
                    ft_putstr_fd("minishell: command not found: ", 2);
                    ft_putendl_fd(pipeline->commands[i]->args[0], 2);
                    exit(127);
                }
                
                char **env_array = env_to_array(shell->env);
                if (!env_array)
                {
                    free(exec_path);
                    exit(1);
                }
                
                execve(exec_path, pipeline->commands[i]->args, env_array);
                
                // If execve returns, there was an error
                perror(exec_path);
                free(exec_path);
                free_env_array(env_array);
                exit(1);
            }
        }
        
        // Parent process
        pids[i] = pid;
        
        // Close pipe ends that are no longer needed
        if (i > 0)
            close(in_fd);
            
        if (i < pipeline->cmd_count - 1)
            close(pipefds[active_pipe][1]);  // Close write end after it's been duplicated
        
        // Update input for next command
        if (i < pipeline->cmd_count - 1)
        {
            in_fd = pipefds[active_pipe][0]; // Use pipe for next command's input
            active_pipe = 1 - active_pipe;  // Swap to the other pipe set
        }
    }
    
    // Close any remaining pipe file descriptors and heredoc FDs
    for (i = 0; i < pipeline->cmd_count; i++)
    {
        if (heredoc_fds[i] != STDIN_FILENO)
            close(heredoc_fds[i]);
    }
    free(heredoc_fds);
    
    if (pipeline->cmd_count > 1)
    {
        if (in_fd != STDIN_FILENO)
            close(in_fd);
    }
    
    // Wait for all processes to complete, starting from the last
    for (i = pipeline->cmd_count - 1; i >= 0; i--)
    {
        int status;
        waitpid(pids[i], &status, 0);
        
        // Get the exit status of the last command in the pipeline
        if (i == pipeline->cmd_count - 1)
        {
            if (WIFEXITED(status))
                exit_status = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                exit_status = 128 + WTERMSIG(status);
            else
                exit_status = 1;
        }
    }
    
    free(pids);
    return exit_status;
}
