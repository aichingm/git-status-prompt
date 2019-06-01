SHELL = /bin/sh

CC = gcc
LIBS = -l git2
FLAGS = -Wall -std=c99 -ansi -pedantic -D_POSIX_C_SOURCE=200809L

.PHONY: build

build:
	$(CC) gitprompt.c -o git_status_prompt $(LIBS) $(FLAGS)
