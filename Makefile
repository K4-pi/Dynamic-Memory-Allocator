CC = gcc
CFLAGS = -Wall -Wextra

SRC := $(wildcard src/*.c)
INCLUDE := -Iinclude

all: compile 

debug:
	$(CC) $(CFLAGS) -o malloc_gdb test.c -ggdb $(SRC) $(INCLUDE)

compile:
	$(CC) $(CFLAGS) -o malloc test.c $(SRC) $(INCLUDE)


