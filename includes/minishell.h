/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   minishell.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/18 13:20:17 by rboland           #+#    #+#             */
/*   Updated: 2025/03/18 13:20:17 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MINISHELL_H
# define MINISHELL_H

# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <signal.h>
# include <sys/wait.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <dirent.h>
# include <errno.h>
# include <limits.h>
# include <readline/readline.h>
# include <readline/history.h>
# include "../libft/includes/libft.h"

// Token types
# define TOKEN_WORD      0  // Regular word/command/argument
# define TOKEN_PIPE      1  // |
# define TOKEN_REDIR_IN  2  // <
# define TOKEN_REDIR_OUT 3  // >
# define TOKEN_APPEND    4  // >>
# define TOKEN_HEREDOC   5  // <<

// Global signal variable
extern volatile sig_atomic_t g_signal;

// Structures
typedef struct s_token
{
    char            *value;
    int             type;
    struct s_token  *next;
} t_token;

typedef struct s_command
{
    char    **args;          // Command and its arguments
    char    *input_file;     // Input redirection
    char    *output_file;    // Output redirection
    int     append_output;   // Flag for >> redirection
    char    *heredoc_delim;  // Delimiter for heredoc
} t_command;

typedef struct s_pipeline
{
    t_command   **commands;  // Array of commands
    int         cmd_count;   // Number of commands
} t_pipeline;

// Main functions
void    process_input(char *input);
void    signal_handler(int signum);

// Parsing functions
t_token *tokenize_input(char *input);
void    free_tokens(t_token *tokens);
t_pipeline *parse_tokens(t_token *tokens);
void    free_pipeline(t_pipeline *pipeline);

// Execution functions
int     execute_pipeline(t_pipeline *pipeline);
int     execute_command(t_command *cmd, int in_fd, int out_fd);
int     execute_builtin(t_command *cmd);
int     is_builtin(char *cmd);

// Built-ins
int     ft_echo(t_command *cmd);
int     ft_cd(t_command *cmd);
int     ft_pwd(void);
int     ft_export(t_command *cmd);
int     ft_unset(t_command *cmd);
int     ft_env(void);
int     ft_exit(t_command *cmd);

// Utils
char    *get_prompt(void);
char    **split_args(char *str);
void    free_split(char **split);

#endif