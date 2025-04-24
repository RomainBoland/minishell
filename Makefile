NAME = minishell

CC = gcc
CFLAGS = -Wall -Wextra -Werror -I./includes -I./libft
LDFLAGS = -lreadline

SRC_DIR = srcs
INC_DIR = includes
OBJ_DIR = objs
LIBFT_DIR = libft

SRCS = $(SRC_DIR)/minishell.c \
	   $(SRC_DIR)/signal/signal.c \
       $(SRC_DIR)/tokenizer/tokenizer.c \
	   $(SRC_DIR)/tokenizer/tokenizer_utils.c \
	   $(SRC_DIR)/tokenizer/tokenizer_heredoc.c \
	   $(SRC_DIR)/tokenizer/tokenize_input.c \
	   $(SRC_DIR)/tokenizer/extract_word.c \
	   $(SRC_DIR)/utils/utils.c \
	   $(SRC_DIR)/process/process.c \
	   $(SRC_DIR)/process/syntax_validation.c \
	   $(SRC_DIR)/process/validation_utils.c \
       $(SRC_DIR)/parser.c \
       $(SRC_DIR)/execute.c \
       $(SRC_DIR)/builtin.c \
	   $(SRC_DIR)/environment.c \
	   $(SRC_DIR)/expand.c	\


OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
LIBFT = $(LIBFT_DIR)/libft.a

all: libft_make $(NAME)

libft_make:
	$(MAKE) -C $(LIBFT_DIR)

$(NAME): $(OBJS) $(LIBFT)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS) $(LIBFT) $(LDFLAGS)

# Compilation des .o en créant les dossiers si besoin
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	$(MAKE) -C $(LIBFT_DIR) clean

fclean: clean
	rm -f $(NAME)
	$(MAKE) -C $(LIBFT_DIR) fclean

re: fclean all

.PHONY: all clean fclean re libft_make
