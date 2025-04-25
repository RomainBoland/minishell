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

// Exit code
# define EXIT_SUCCESS 0
# define EXIT_FAILURE 1
# define EXIT_INVALID_USAGE 2
# define EXIT_NOT_EXECUTABLE 126
# define EXIT_NOT_FOUND 127

// Global signal variable
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
    char                    *file;
    int                     type;  // REDIR_IN, REDIR_OUT, APPEND, HEREDOC
    int                     quoted;
    struct s_redirection    *next;
};

struct s_command
{
    char			**args;
    int				*arg_quoted;
    t_redirection	*redirections;  // Linked list of redirections
    char			**heredoc_delims;
	int				heredoc_count;
    int             *heredoc_quoted;
	int				has_heredoc;
    t_token         *token;
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

/* ---- minishell.c ---- */
void	process_input(char *input, t_shell *shell);

/* ---- utils.c ---- */
char	*get_prompt(void);
int		ft_strcmp(char *s1, char *s2);
int		has_unclosed_quotes(char *input);

/* ---- environment.c ---- */
t_env	*init_env(char **envp);
void	free_env(t_env *env);
char	*get_env_value(t_env *env, char *key);
void	set_env_value(t_env *env, char *key, char *value);
void	remove_env_var(t_env *env, char *key);
char	**env_to_array(t_env *env);
void	free_env_array(char **env_array);
int		has_env_key(t_env *env, char *key);
char	**env_to_array_export(t_env *env);

/* ---- builtin.c ---- */
int		ft_echo(t_command *cmd);
int		ft_cd(t_command *cmd, t_shell *shell);
int		ft_pwd(void);
int		ft_export(t_command *cmd, t_shell *shell);
int		ft_unset(t_command *cmd, t_shell *shell);
int		ft_env(t_shell *shell);
int		ft_exit(t_command *cmd, t_shell *shell);

/* ---- tokenizer.c ---- */
t_token	*new_token(char *value, int type, int quoted_state);
void	add_token(t_token **tokens, t_token *new);
char	*extract_quoted_str(char *input, int *i, char quote_char);

/* ---- tokenizer_utils.c ---- */
void	process_pipe_token(t_token **tokens, int *i);
void	process_input_redir(t_token **tokens, char *input, int *i);
void	process_output_redir(t_token **tokens, char *input, int *i);
void	process_quoted_heredoc(t_token **tokens, char *in, int *i, int q_type);
void	process_unquoted_heredoc(t_token **tokens, char *input, int *i);

/* ---- tokenizer_heredoc.c ---- */
void	skip_whitespace(char *input, int *i);
void	process_heredoc(t_token **tokens, char *input, int *i);
void	process_word_token(t_token **tokens, char *input, int *i);
void	free_tokens(t_token *tokens);

/* ---- extract_word.c ---- */
char	*extract_word(char *input, int *i);

/* ---- tokenize_input.c ---- */
t_token	*tokenize_input(char *input);

/* ---- syntax_validation.c ---- */
int		validate_syntax(t_token *tokens);
int		check_consecutive_redirections(t_token *tokens);

/* ---- validation_utils.c ---- */
int		is_redirection(int token_type);
int		check_pipe_errors(t_token *current, t_token *prev);
void	print_redir_error(int token_type);
int		handle_missing_word_error(t_token *current);

/* ---- parser.c ---- */
t_pipeline	*parse_tokens(t_token *tokens);
void		free_pipeline(t_pipeline *pipeline);

/* ---- expand_variables.c ---- */
char	*expand_variables(char *str, t_shell *shell);

/* ---- expand_utils.c ---- */
int		is_valid_var_char(char c, int first_char);
char	*extract_var_name(char *str, int *i);
char	*expand_single_var(char *str, int *i, t_shell *shell);
int		is_in_dquotes(char *str, int pos);
int		should_expand_var(char *str, int i);

/* ---- expand_cmd.c ---- */
void	expand_command_args(t_command *cmd, t_shell *shell);
void	expand_redirections(t_command *cmd, t_shell *shell);
void	expand_heredoc_delims(t_command *cmd, t_shell *shell);
void	expand_pipeline(t_pipeline *pipeline, t_shell *shell);
char	*process_dquote_char(char *result, char c, int *i);

/* ---- signal.c ---- */
void	setup_signals(void);

/* ---- execute_core.c ---- */
int		is_builtin(char *cmd);
int		execute_builtin(t_command *cmd, t_shell *shell);
char	*find_executable(char *cmd, t_shell *shell);
char	*find_exec_in_path(char *path_dir, char *cmd);
void	free_paths(char **paths);

/* ---- execute_setup.c ---- */
void	handle_heredoc_signals(struct sigaction *old_int, 
		struct sigaction *old_quit);
void	setup_child_signals(void);
void	process_heredoc_lines(t_command *cmd, int pipe_fd[2], t_shell *shell);
char	*append_heredoc_line(char *collected, char *line, int quoted, 
		t_shell *shell);

/* ---- execute_heredoc.c ---- */
int		setup_heredoc(t_command *cmd, t_shell *shell);
int		handle_heredoc_fork_error(int pipe_fd[2], struct sigaction *old_int, 
		struct sigaction *old_quit);
int		handle_heredoc_parent(pid_t pid, int pipe_fd[2], 
		struct sigaction *old_int, struct sigaction *old_quit);
int		execute_command(t_command *cmd, int in_fd, int out_fd, t_shell *shell);
int		handle_command_heredoc(t_command *cmd, t_shell *shell);

/* ---- execute_redirect.c ---- */
int		update_input_fd(t_command *cmd, int in_fd, int heredoc_fd);
int		execute_builtin_with_redirects(t_command *cmd, int in_fd, 
		int out_fd, t_shell *shell);
void	restore_std_fds(int stdin_copy, int stdout_copy);
int		execute_external_command(t_command *cmd, int in_fd, int out_fd, 
		t_shell *shell, int heredoc_fd);
int		setup_redirections(t_command *cmd, int in_fd, int out_fd);

/* ---- execute_process.c ---- */
int		handle_fork_error(char *exec_path, int heredoc_fd);
int		setup_cmd_redirections(t_command *cmd);
int		setup_input_redir(t_redirection *redir);
int		setup_output_redir(t_redirection *redir, int mode);
void	child_process(t_command *cmd, int in_fd, int out_fd, 
		char *exec_path, t_shell *shell);

/* ---- execute_process_utils.c ---- */
void	setup_child_process_signals(t_command *cmd);
void	close_unneeded_fds(int in_fd, int out_fd);
void	execute_child_command(t_command *cmd, char *exec_path, t_shell *shell);
int		parent_process(pid_t pid, char *exec_path, t_command *cmd, 
		int heredoc_fd);
void	handle_minishell_parent(t_command *cmd, pid_t pid);
void	restore_after_minishell(t_command *cmd);

/* ---- execute_pipeline.c ---- */
int		execute_pipeline(t_pipeline *pipeline, t_shell *shell);
int		is_single_builtin(t_pipeline *pipeline);
int		setup_heredocs(t_pipeline *pipeline, t_shell *shell, int *heredoc_fds);
void	cleanup_heredocs(int *heredoc_fds, int count);
int		execute_pipeline_commands(t_pipeline *pipeline, t_shell *shell,
		pid_t *pids, int *heredoc_fds);

/* ---- execute_pipeline_utils.c ---- */
int		handle_pipe_error(int *heredoc_fds, int cmd_count);
int		execute_pipeline_command(t_pipeline *pipeline, t_shell *shell, 
		int i, int *active_pipe, int pipefds[2][2], int in_fd, 
		pid_t *pids, int *heredoc_fds);
void	close_unused_pipes(t_pipeline *pipeline, int i, int active_pipe, 
		int pipefds[2][2]);
void	setup_pipe_redirects(int in_fd, int out_fd);
void	execute_pipeline_command_child(t_pipeline *pipeline, t_shell *shell,
		int i, int *heredoc_fds);
void	handle_parent_pipes_fixed(int i, int in_fd, int active_pipe,
		int pipefds[2][2], t_pipeline *pipeline);

/* ---- execute_pipeline_process.c ---- */
pid_t	create_command_process(t_pipeline *pipeline, t_shell *shell, int i,
		int in_fd, int out_fd, int *heredoc_fds);
void	execute_pipeline_child(t_pipeline *pipeline, t_shell *shell, int i,
		int in_fd, int out_fd, int *heredoc_fds);
void	handle_child_redirections(t_pipeline *pipeline, int i, int in_fd,
		int out_fd, int *heredoc_fds);
int		handle_input_redirections(t_command *cmd);
void	setup_command_input(int i, int in_fd, int *heredoc_fds);

/* ---- execute_pipeline_process2.c ---- */
void	handle_output_redirections(t_command *cmd, int out_fd);
void	close_child_pipes(int cmd_count, int i);
void	execute_child_process(t_pipeline *pipeline, t_shell *shell, int i);
void	handle_parent_pipes(int i, int in_fd, int active_pipe,
		int pipefds[2][2], t_pipeline *pipeline);
void	wait_for_processes(pid_t *pids, int count);

/* ---- execute_pipeline_wait.c ---- */
int		wait_for_pipeline(pid_t *pids, int cmd_count);

#endif