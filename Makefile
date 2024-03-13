CC=gcc

run: main
	./main

main: main.o
	$(CC) -o main main.o

%.o: %.c
	$(CC) -o $@ -c $^ -Wall -Wextra -pedantic -std=c99
