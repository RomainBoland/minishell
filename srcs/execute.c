/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/19 11:05:20 by rboland           #+#    #+#             */
/*   Updated: 2025/03/29 11:58:42 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

int setup_heredoc(t_command *cmd, t_shell *shell)
{
    int     pipe_fd[2];
    char    *line;
    pid_t   pid;
    int     status;
    
    if (!cmd->heredoc_delim || !cmd->has_heredoc)
        return STDIN_FILENO;
    
    if (pipe(pipe_fd) < 0)
    {
        perror("pipe");
        return -1;
    }
    
    // Fork to handle signals properly in heredoc
    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return -1;
    }
    
    if (pid == 0) // Child process
    {
        // Set up signal handling for child (heredoc reader)
        signal(SIGINT, SIG_DFL);  // Default behavior for Ctrl+C
        
        close(pipe_fd[0]);  // Close read end in child
        
        // Read lines until delimiter is found
        while (1)
        {
            line = readline("> ");  // Prompt for heredoc input
            
            if (!line || ft_strcmp(line, cmd->heredoc_delim) == 0)
            {
                free(line);
                break;
            }
            
            // Expand variables in the heredoc line if not in single quotes
            char *expanded = expand_variables(line, shell);
            if (expanded)
            {
                // Write to pipe and add newline
                write(pipe_fd[1], expanded, ft_strlen(expanded));
                write(pipe_fd[1], "\n", 1);
                free(expanded);
            }
            else
            {
                // Fallback to original line if expansion fails
                write(pipe_fd[1], line, ft_strlen(line));
                write(pipe_fd[1], "\n", 1);
            }
            
            free(line);
        }
        
        close(pipe_fd[1]);
        exit(0);
    }
    else // Parent process
    {
        close(pipe_fd[1]);  // Close write end in parent
        
        // Wait for child to complete
        waitpid(pid, &status, 0);
        
        // Check if child was terminated by a signal
        if (WIFSIGNALED(status))
        {
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
    
    // Handle empty command
    if (!cmd || !cmd->args || !cmd->args[0])
        return 0;
    
    // Set up heredoc if needed
    if (cmd->heredoc_delim)
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
    
    // The rest of your execute_command function remains the same...
    
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
        // This will only be called for a single external command
        // Fork and execute
        pid_t pid;
        
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
            // Set up signal handlers for child
            signal(SIGINT, SIG_DFL);    // Default Ctrl+C behavior
            signal(SIGQUIT, SIG_DFL);   // Default Ctrl+\ behavior
            
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
        
        // Close heredoc fd in parent since it's duplicated in child
        if (heredoc_fd != STDIN_FILENO)
            close(heredoc_fd);
        
        // Wait for child to finish
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status))
            return WEXITSTATUS(status);
        else if (WIFSIGNALED(status))
            return 128 + WTERMSIG(status);
        
        return 1;
    }
}

// Execute a pipeline of commands
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
    
    // Determine initial input for first command
    t_redirection *first_input = NULL;
    if (pipeline->commands[0]->redirections)
    {
        t_redirection *temp = pipeline->commands[0]->redirections;
        while (temp)
        {
            if (temp->type == TOKEN_REDIR_IN)
                first_input = temp;  // Find the last input redirection
            temp = temp->next;
        }
    }

    // Start with standard input or heredoc for first command
    in_fd = (first_input) ? STDIN_FILENO : heredoc_fds[0];
    
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
                    for (int j = 0; j < pipeline->cmd_count; j++)
                    {
                        if (heredoc_fds[j] != STDIN_FILENO)
                            close(heredoc_fds[j]);
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
            for (int j = 0; j < pipeline->cmd_count; j++)
            {
                if (heredoc_fds[j] != STDIN_FILENO)
                    close(heredoc_fds[j]);
            }
            free(pids);
            free(heredoc_fds);
            return 1;
        }
        
        if (pid == 0)  // Child process
        {
            // Child doesn't need to keep track of pids or heredoc_fds
            free(pids);
            
            // Set up signal handlers for child
            signal(SIGINT, SIG_DFL);    // Default Ctrl+C behavior
            signal(SIGQUIT, SIG_DFL);   // Default Ctrl+\ behavior
            
            // If not first command, set input from previous pipe
            if (i > 0)
            {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            else if (first_input)
            {
                // Handle input file for first command if it exists
                int fd = open(first_input->file, O_RDONLY);
                if (fd < 0)
                {
                    perror(first_input->file);
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            else if (heredoc_fds[i] != STDIN_FILENO)
            {
                // Use heredoc as input if available
                dup2(heredoc_fds[i], STDIN_FILENO);
                close(heredoc_fds[i]);
            }
            
            // Close all heredoc FDs in child
            for (int j = 0; j < pipeline->cmd_count; j++)
            {
                if (heredoc_fds[j] != STDIN_FILENO && heredoc_fds[j] != in_fd)
                    close(heredoc_fds[j]);
            }
            free(heredoc_fds);
            
            // If not last command, set output to next pipe
            if (i < pipeline->cmd_count - 1)
            {
                dup2(out_fd, STDOUT_FILENO);
                close(pipefds[active_pipe][0]);
                close(pipefds[active_pipe][1]);
                
                // Close the other set of pipes if they exist
                if (i < pipeline->cmd_count - 2)
                {
                    close(pipefds[1 - active_pipe][0]);
                    close(pipefds[1 - active_pipe][1]);
                }
            }
            
            // Process all redirections for this command in order
            t_redirection *redir = pipeline->commands[i]->redirections;
            while (redir)
            {
                if (redir->type == TOKEN_REDIR_IN)
                {
                    int fd = open(redir->file, O_RDONLY);
                    if (fd < 0)
                    {
                        perror(redir->file);
                        exit(1);
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
                else if (redir->type == TOKEN_REDIR_OUT)
                {
                    int fd = open(redir->file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0)
                    {
                        perror(redir->file);
                        exit(1);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
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
                }
                redir = redir->next;
            }
            
            // Execute command
            if (is_builtin(pipeline->commands[i]->args[0]))
            {
                // For built-ins in a pipeline, we still need to exit with the correct status
                exit(execute_builtin(pipeline->commands[i], shell));
            }
            else
            {
                // For external commands
                char *exec_path = find_executable(pipeline->commands[i]->args[0], shell);
                if (!exec_path)
                {
                    ft_putstr_fd("minishell: command not found: ", 2);
                    ft_putendl_fd(pipeline->commands[i]->args[0], 2);
                    exit(127);  // Command not found exit status
                }
                
                // Convert environment to array for execve
                char **env_array = env_to_array(shell->env);
                if (!env_array)
                {
                    free(exec_path);
                    exit(1);
                }
                
                // Execute command
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
            // Check if next command has input redirection
            t_redirection *next_input = NULL;
            t_redirection *temp = pipeline->commands[i+1]->redirections;
            while (temp)
            {
                if (temp->type == TOKEN_REDIR_IN)
                    next_input = temp;  // Find the last input redirection
                temp = temp->next;
            }
            
            if (next_input)
                in_fd = STDIN_FILENO;  // Will be handled in child
            else if (heredoc_fds[i+1] != STDIN_FILENO)
                in_fd = heredoc_fds[i+1];  // Use heredoc
            else
                in_fd = pipefds[active_pipe][0]; // Use pipe
                
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
