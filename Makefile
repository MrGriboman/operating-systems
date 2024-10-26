all: test

test: bg_start.c
	gcc bg_start.c -I . -Wall -o test