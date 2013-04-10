# This is part of coursework for Computer Network, 2013.02-2013.06
# By Pengyu CHEN(cpy.prefers.you@gmail.com)
# COPYLEFT, ALL WRONGS RESERVED.

CC = gcc
CCFLAG = -g -c -O2 -m32
LD = gcc
LDFLAG = -g -O2 -m32

#.PHONY: clean

all: ftp16

ftp16: ftp16.o
	$(LD) $(LDFLAG) -o ftp16 ftp16.o

ftp16.o: ftp16.c
	$(CC) $(CCFLAG) -o ftp16.o ftp16.c
		
clean:
	rm -f ftp16.o ftp16
	
