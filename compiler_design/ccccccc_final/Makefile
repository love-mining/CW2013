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
LEXFLAGS = --verbose
YACCFLAGS = -d --verbose --debug

# override the buildin rules by GNU make
%.c: %.y
%.c: %.l

.PHONY: wc clean

all: ccccccc tm

tm: tm.c
	$(CC) tm.c -o tm

ccccccc: ccccccc.tab.o ccccccc.yy.o ccccccc.o
	$(LD) $(LDFLAGS) -o ccccccc ccccccc.tab.o ccccccc.yy.o ccccccc.o

ccccccc.o: ccccccc.c ccccccc.yy.h ccccccc.tab.h ccccccc.h
	$(CC) $(CCFLAGS) -o ccccccc.o ccccccc.c

ccccccc.tab.o: ccccccc.tab.c
	$(CC) $(CCFLAGS) -o ccccccc.tab.o ccccccc.tab.c

ccccccc.tab.h ccccccc.tab.c: ccccccc.y ccccccc.h
	$(YACC) $(YACCFLAGS) -o ccccccc.tab.c ccccccc.y

ccccccc.yy.o: ccccccc.yy.c
	$(CC) $(CCFLAGS) -o ccccccc.yy.o ccccccc.yy.c

ccccccc.yy.h ccccccc.yy.c: ccccccc.lex ccccccc.tab.h ccccccc.h
	$(LEX) $(LEXFLAGS) --header-file=ccccccc.yy.h -o ccccccc.yy.c ccccccc.lex

wc:
	wc ccccccc.y ccccccc.lex ccccccc.c

clean:
	rm -f tm ccccccc ccccccc.tab.o ccccccc.yy.o ccccccc.tab.c ccccccc.tab.h ccccccc.yy.c ccccccc.yy.h ccccccc.o


