SHELL = /bin/sh

CC = gcc
LIBS = -l git2
FLAGS = -Wall -std=c99 -ansi -pedantic

.PHONY: build

build:
	$(CC) gitprompt.c -o git_status_prompt $(LIBS) $(FLAGS)
