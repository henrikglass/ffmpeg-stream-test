
all:
	gcc -Wall -Wextra -Wpedantic -O0 -ggdb3 -Iinclude src/player.c -o player -Llib -lraylib -lm -lavcodec -lavformat -lswscale -lavutil -lpthread -ldl

clean:
	-rm player

