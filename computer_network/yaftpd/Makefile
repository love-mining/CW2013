# This is part of coursework for Computer Network, 2013.02-2013.06
# By Pengyu CHEN(cpy.prefers.you@gmail.com)
# COPYLEFT, ALL WRONGS RESERVED.

CC = gcc 
CCFLAG = -g -c -O2
LD = gcc
LDFLAG = -g -lconfig -O2

#.PHONY: clean

all: yaftpd

yaftpd: yaftpd.o
	$(LD) $(LDFLAG) -o yaftpd yaftpd.o

yaftpd.o: yaftpd.c 
	$(CC) $(CCFLAG) -o yaftpd.o yaftpd.c
		
clean:
	rm -f yaftpd.o yaftpd
	
