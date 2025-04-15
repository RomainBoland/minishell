/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   minishell.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rboland <rboland@student.s19.be>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/18 13:24:55 by rboland           #+#    #+#             */
/*   Updated: 2025/03/18 13:24:55 by rboland          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

volatile sig_atomic_t g_signal = 0;

// Signal handler for interactive mode
// when pressing ctrl+c in heredoc, should display > ^C but we just get a newline instead
void signal_handler(int signum)
{
    g_signal = signum;
    
    if (signum == SIGINT) // Ctrl+C
    {
        // Just write a newline and set flag - readline will handle redisplay
        write(STDOUT_FILENO, "\n", 1);
        
        // Only replace line and redisplay if we're in readline context
        if (rl_readline_state & RL_STATE_READCMD) {
            rl_on_new_line();
            rl_replace_line("", 0);
            rl_redisplay();
        }
    }
}

// Signal handler for child processes
void signal_handler_child(int signum)
{
    if (signum == SIGINT) // Ctrl+C
        exit(130);        // Bash exits with 130 on SIGINT
    if (signum == SIGQUIT) // Ctrl+'\'
        exit(131);        // Bash exits with 131 on SIGQUIT
}

void setup_signals(void)
{
    struct sigaction sa_int, sa_quit;
    
    // Initialize the sigaction structs
    ft_memset(&sa_int, 0, sizeof(sa_int));
    ft_memset(&sa_quit, 0, sizeof(sa_quit));
    
    // Setup SIGINT handler (Ctrl+C)
    sa_int.sa_handler = signal_handler;
    sigaction(SIGINT, &sa_int, NULL);
    
    // Ignore SIGQUIT (Ctrl+\) in interactive mode
    sa_quit.sa_handler = SIG_IGN;
    sigaction(SIGQUIT, &sa_quit, NULL);
}

char *get_prompt(void)
{
    char *username;
    char hostname[256];
    char cwd[PATH_MAX];
    char *prompt;

    username = getenv("USER");
    if (!username)
        username = "user";
    if (gethostname(hostname, sizeof(hostname)) != 0)
        ft_strlcpy(hostname, "localhost", sizeof(hostname));
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        ft_strlcpy(cwd, "~", sizeof(cwd));
    prompt = malloc(ft_strlen(username) + ft_strlen(hostname) + ft_strlen(cwd) + 50);
    if (!prompt)
        return (ft_strdup("minishell$ "));
    sprintf(prompt, "\033[1;36m%s@%s:%s$\033[0m ", username, hostname, cwd);  
    return (prompt);
}

int main(int argc, char **argv, char **envp)
{
    char *input;
    char *prompt;
    t_shell shell;
    
    if (argc != 1)
    {
        printf("minishell: %s: No such file or directory\n", argv[1]);
        return (127);
    }
    (void)argv;
    
    shell.env = init_env(envp);
    shell.last_exit_status = 0;
    setup_signals();
	printf("	\033[33mBienvenue dans minishell\033[33m\n");
    while (1)
    {
        g_signal = 0;
        prompt = get_prompt();
        input = readline(prompt);
        if (!input)
        {
            write(STDOUT_FILENO, "exit\n", 5);
            free(prompt);
            break;
        }
        if (input[0] != '\0')
        {
            add_history(input);
            process_input(input, &shell);
        }
        free(prompt);
        free(input);
    }
    rl_clear_history();
    free_env(shell.env);
    return (shell.last_exit_status);
}
