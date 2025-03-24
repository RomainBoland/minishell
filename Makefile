NAME = minishell

CC = gcc
CFLAGS = -Wall -Wextra -Werror -I./includes -I./libft
LDFLAGS = -lreadline

SRC_DIR = srcs
INC_DIR = includes
OBJ_DIR = objs
LIBFT_DIR = libft

SRCS = $(SRC_DIR)/minishell.c \
       $(SRC_DIR)/tokenizer.c \
       $(SRC_DIR)/parser.c \
       $(SRC_DIR)/process.c \
       $(SRC_DIR)/execute.c \
       $(SRC_DIR)/builtin.c \
	   $(SRC_DIR)/environment.c \
	   $(SRC_DIR)/expand.c	\
	   $(SRC_DIR)/utils.c

OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
LIBFT = $(LIBFT_DIR)/libft.a

all: libft_make $(OBJ_DIR) $(NAME)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

libft_make:
	$(MAKE) -C $(LIBFT_DIR)

$(NAME): $(OBJS) $(LIBFT)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS) $(LIBFT) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	$(MAKE) -C $(LIBFT_DIR) clean

fclean: clean
	rm -f $(NAME)
	$(MAKE) -C $(LIBFT_DIR) fclean

re: fclean all

.PHONY: all clean fclean re libft_make