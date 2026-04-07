CC = gcc
CFLAGS = -Wall -Wextra

SRC := $(wildcard src/*.c)
INCLUDE := -Iinclude

all: debug 

debug:
	$(CC) $(CFLAGS) -o malloc test.c -ggdb $(SRC) $(INCLUDE)
