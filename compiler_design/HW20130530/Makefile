# This is part of the coursework for Compiler Design, 2013.05-06
# by Pengyu CHEN (cpy.prefers.you[at]gmail.com)
# COPYLEFT, ALL WRONGS RESERVED.
 
# oops this makefile sucks..

LD = gcc
CC = gcc
LEX = flex
YACC = bison

LGFLAGS = 
CCFLAGS = -O2 -c
LEXFLAGS =
YACCFLAGS = -d

# override the buildin rules by GNU make
%.c: %.y
%.c: %.l

.PHONY: wc clean

all: E6_30

E6_30: E6_30.tab.o E6_30.yy.o E6_30.o
	$(LD) $(LDFLAGS) -o E6_30 E6_30.tab.o E6_30.yy.o E6_30.o

E6_30.o: E6_30.c E6_30.yy.h E6_30.tab.h
	$(CC) $(CCFLAGS) -o E6_30.o E6_30.c

E6_30.tab.o: E6_30.tab.c
	$(CC) $(CCFLAGS) -o E6_30.tab.o E6_30.tab.c

E6_30.tab.h E6_30.tab.c: E6_30.y
	$(YACC) $(YACCFLAGS) -o E6_30.tab.c E6_30.y

E6_30.yy.o: E6_30.yy.c
	$(CC) $(CCFLAGS) -o E6_30.yy.o E6_30.yy.c

E6_30.yy.h E6_30.yy.c: E6_30.lex E6_30.tab.h
	$(LEX) $(LEXFLAGS) --header-file=E6_30.yy.h -o E6_30.yy.c E6_30.lex

wc:
	wc E6_30.y E6_30.lex E6_30.c

clean:
	rm -f E6_30 E6_30.tab.o E6_30.yy.o E6_30.tab.c E6_30.tab.h E6_30.yy.c E6_30.yy.h E6_30.o


