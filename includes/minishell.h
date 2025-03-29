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
# include <string.h>
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

// Global signal variable - this is the only global we're allowed to use
extern volatile sig_atomic_t g_signal;

// Forward declarations
typedef struct s_token t_token;
typedef struct s_command t_command;
typedef struct s_pipeline t_pipeline;
typedef struct s_env t_env;
typedef struct s_shell t_shell;
typedef struct s_redirection t_redirection;

// Structures
struct s_token
{
    char            *value;
    int             type;
    int             quoted_state; // 0 = unquoted, 1 = single quoted, 2 = double quoted
    struct s_token  *next;
};

struct s_redirection
{
    char *file;
    int type;  // REDIR_IN, REDIR_OUT, APPEND, HEREDOC
    struct s_redirection *next;
};

struct s_command
{
    char			**args;
    int				*arg_quoted;
    t_redirection	*redirections;  // Linked list of redirections
    char			*heredoc_delim;  // Keep this for convenience
	int				has_heredoc;
};

struct s_pipeline
{
    t_command   **commands;  // Array of commands
    int         cmd_count;   // Number of commands
};

struct s_env
{
    char            *key;
    char            *value;
    struct s_env    *next;
};

struct s_shell
{
    t_env   *env;            // Environment variables
    int     last_exit_status; // Exit status of last command
};

// Main functions
void    	process_input(char *input, t_shell *shell);
void    	signal_handler(int signum);
void    	setup_signals(void);

// Parsing functions
t_token 	*tokenize_input(char *input);
void		free_tokens(t_token *tokens);
t_pipeline *parse_tokens(t_token *tokens);
void    	free_pipeline(t_pipeline *pipeline);

// Execution functions
int         setup_heredoc(t_command *cmd, t_shell *shell);
int     	execute_pipeline(t_pipeline *pipeline, t_shell *shell);
int     	execute_command(t_command *cmd, int in_fd, int out_fd, t_shell *shell);
int     	execute_builtin(t_command *cmd, t_shell *shell);
int     	is_builtin(char *cmd);

// Built-ins
int     	ft_echo(t_command *cmd);
int     	ft_cd(t_command *cmd, t_shell *shell);
int    		ft_pwd(void);
int     	ft_export(t_command *cmd, t_shell *shell);
int     	ft_unset(t_command *cmd, t_shell *shell);
int     	ft_env(t_shell *shell);
int     	ft_exit(t_command *cmd, t_shell *shell);

// Environment functions
t_env   	*init_env(char **envp);
void    	free_env(t_env *env);
char    	*get_env_value(t_env *env, char *key);
void    	set_env_value(t_env *env, char *key, char *value);
void    	remove_env_var(t_env *env, char *key);
char    	**env_to_array(t_env *env);
void    	free_env_array(char **env_array);
t_env   	*create_env_node(char *key, char *value);

// Environment variable expansion
char    	*expand_variables(char *str, t_shell *shell);
void    	expand_command_args(t_command *cmd, t_shell *shell);
void   		expand_pipeline(t_pipeline *pipeline, t_shell *shell);
int     	is_in_dquotes(char *str, int pos);

// Utils
char    	*get_prompt(void);
int			ft_strcmp(char *s1, char *s2);
// char    **split_args(char *str);
// void    free_split(char **split);

#endif