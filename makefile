CC=gcc
CFLAGS=-O3 -Wall -Werror

all:	zmtx zmrx

zmtx:	zmtx.o zmdm.o crctab.o unixterm.o
	$(CC) $(CFLAGS) $(OFLAG) zmtx.o zmdm.o crctab.o unixterm.o -o zmtx

zmrx:	zmrx.o zmdm.o crctab.o unixterm.o
	$(CC) $(CFLAGS) $(OFLAG) zmrx.o zmdm.o crctab.o unixterm.o -o zmrx

zmtx.o:		zmtx.c
zmrx.o:		zmrx.c

zmdm.o:		zmdm.c
crctab.o:	crctab.c

unixterm.o:	unixterm.c 

clean:
	rm -f *.o
	rm -f zmtx zmrx


