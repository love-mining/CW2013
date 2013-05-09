# This is part of the coursework for Compiler Design, 2013.05-06
# by Pengyu CHEN (cpy.prefers.you[at]gmail.com)
# COPYLEFT, ALL WRONGS RESERVED.
 
# oops this makefile sucks..

CC = gcc
LEX = flex
YACC = bison

CFLAGS = -O2
LEXFLAGS =
YACCFLAGS = 

# override the buildin rules by GNU make
%.c: %.y
%.c: %.l

.PHONY: wc clean

all: E2_22 E2_24

E2_22: E2_22.c
	$(CC) $(CFLAGS) -o E2_22 E2_22.c

E2_22.c: E2_22.lex
	$(LEX) $(LEXFLAGS) -o E2_22.c E2_22.lex

E2_24: E2_24.c
	$(CC) $(CFLAGS) -o E2_24 E2_24.c

E2_24.c: E2_24.lex
	$(LEX) $(LEXFLAGS) -o E2_24.c E2_24.lex

wc:
	wc E2_22.lex E2_24.lex

clean:
	rm -f E2_22 E2_22.c E2_24 E2_24.c


