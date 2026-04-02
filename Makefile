CC = gcc
CFLAGS = -Wall -Wextra

SRC := $(wildcard src/*.c)
INCLUDE := -Iinclude

all: compile 

compile:
	$(CC) $(CFLAGS) -o malloc test.c $(SRC) $(INCLUDE)


