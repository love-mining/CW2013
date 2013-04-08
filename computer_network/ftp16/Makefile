# This is part of coursework for Computer Network, 2013.02-2013.06
# By Pengyu CHEN(cpy.prefers.you@gmail.com)
# COPYLEFT, ALL WRONGS RESERVED.

CC = gcc
CCFLAG = -g -c -O2 -m32
LD = gcc
LDFLAG = -g -O2 -m32

#.PHONY: clean

all: ftp64

ftp64: ftp64.o
	$(LD) $(LDFLAG) -o ftp64 ftp64.o

ftp64.o: ftp64.c
	$(CC) $(CCFLAG) -o ftp64.o ftp64.c
		
clean:
	rm -f ftp64.o ftp64
	
