NAME= ft_ping
NAME_TESTER= pong
SRC= srcs/main.c \
	srcs/common.c

SRC_TESTER= srcs/pong.c \
		srcs/common.c

HDRS= includes/ft_ping.h

OBJ= $(SRC:.c=.o)
OBJ_TESTER= $(SRC_TESTER:.c=.o)

HDR_PATH= ./libft/includes/
CC= gcc
CC_FLAGS= -v -Ofast -Wall -Wextra  #-g3  -fsanitize=address
LIBFT_PATH=./libft
FLAGS= -L$(LIBFT_PATH) -lft -I$(HDR_PATH) -I./includes

all: submodule $(NAME) $(NAME_TESTER)

submodule:
	@make -C libft/

$(NAME): $(OBJ)
	$(CC) $(CC_FLAGS) $^ $(FLAGS) -o $(NAME)

$(NAME_TESTER): $(OBJ_TESTER)
	$(CC) $(CC_FLAGS) $^ $(FLAGS) -o $(NAME_TESTER)

%.o : %.c $(HDRS)
	$(CC) $(CC_FLAGS) $< -c -I$(HDR_PATH) -I./includes -o $@

clean:
	rm -f $(OBJ) $(OBJ_TESTER)
	rm -f ft.log ping.log
	make -C $(LIBFT_PATH) clean
fclean: clean
	rm -f $(NAME) $(NAME_TESTER)
	rm -f ft.log ping.log
	make -C $(LIBFT_PATH) fclean

re: fclean all
