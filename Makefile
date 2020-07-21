SHELL = /bin/sh

CC = gcc
LIBS = -l git2
FLAGS = -Wall -std=c99 -ansi -pedantic

.PHONY: build install-local clear

build:
	$(CC) git_status_prompt.c -o git_status_prompt $(LIBS) $(FLAGS)

install-local: build
	mv git_status_prompt ~/.local/bin
	chmod +x ~/.local/bin/git_status_prompt

clear:
	rm git_status_prompt
