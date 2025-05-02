markdown# Minishell

A simple shell implementation based on bash, developed as part of the 42 School curriculum.

## Overview

Minishell is a simplified version of a Unix shell that supports:
- Command execution
- Pipes (`|`)
- Redirections (`<`, `>`, `>>`, `<<`)
- Environment variables
- Signal handling
- Builtin commands

## Installation

### Prerequisites

- GCC compiler
- Make
- Readline library

### Compilation

```bash
make
```
This will compile the minishell executable.
Usage
```bash
./minishell
```
This will open an interactive shell prompt where you can enter commands.

## Features
### Basic Shell Features

- Command execution: Run any command available in your PATH
- Command pipelines: Connect commands with pipes (e.g., ls | grep txt)
- Input/Output redirections:

	- command < input_file: Input redirection
	- command > output_file: Output redirection
	- command >> output_file: Append output redirection
	- command << DELIMITER: Heredoc input



### Environment Variables

- Use $VAR_NAME to expand environment variables
- Access the last command's exit code using $?
- Single quotes (') prevent variable expansion
- Double quotes (") allow variable expansion

### Builtin Commands
Minishell implements several builtin commands:

- echo [-n] [string ...]: Display a line of text
- cd [directory]: Change the current directory
	- cd without arguments navigates to the home directory
	- cd - returns to the previous directory
	- cd ~ navigates to the home directory

- pwd: Print the current working directory
- export [name[=value] ...]: Set environment variables
- unset [name ...]: Remove environment variables
- env: Display all environment variables
- exit [status]: Exit the shell with an optional status code

### Signal Handling
Minishell handles the following signals:

- Ctrl+C (SIGINT): Interrupts the current command
- Ctrl+\ (SIGQUIT): Ignored in interactive mode, generates core dump in child processes
- Ctrl+D (EOF): Exits the shell

### Architecture
Minishell is built with a clear pipeline architecture:

1. Input Processing: Read user input through the readline library
2. Tokenization: Break input into tokens (words, pipes, redirections)
3. Syntax Validation: Check for syntax errors in the command
4. Parsing: Organize tokens into command structures
5. Expansion: Handle environment variable expansion
6. Execution: Execute commands with the proper redirections and pipes

### Core Components
#### Tokenizer
Breaks the input string into tokens, handling quotes and special characters:

- Words (commands and arguments)
- Pipes (|)
- Redirections (<, >, >>, <<)

#### Parser
Converts tokens into structured commands and pipelines.
#### Expander
Handles environment variable expansion, respecting quotes rules.
#### Executor
Executes commands:

For builtins: Direct execution
For external commands: Path resolution, fork/exec

Examples
```bash
# Basic command execution
ls -la

# Command with pipe
ls -l | grep ".txt"

# Input/output redirections
cat < input.txt > output.txt

# Heredoc
cat << EOF
This is a heredoc example
Multiple lines of text
EOF

# Environment variables
echo $USER
export MY_VAR=value
echo $MY_VAR

# Command sequence
mkdir test_dir && cd test_dir && pwd

# Background process
sleep 10 &
```
### Error Handling
Minishell provides error messages for:

- Syntax errors
- Command not found
- Permission denied
- File not found
- Other execution errors

### Development
File Structure

- minishell.h: Main header file
- structure.h: Data structures definitions
- minishell.c: Program entry point
- tokenizer: Token generation from input
- parser: Command structure building
- expander: Environment variable expansion
- executor: Command execution
- builtins: Builtin command implementation
- signals: Signal handling
- utils: Utility functions

### Memory Management
Minishell carefully manages memory to avoid leaks, freeing all allocated resources when they are no longer needed.

### License
This project is part of the curriculum at 42 School and is subject to their licensing terms.