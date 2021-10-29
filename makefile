CC=gcc
#CFLAGS=-O -DUNITE
CFLAGS=-O -DSUNOS4

all:	zmtx zmrx

zmtx:	zmtx.o zmdm.o crctab.o
	$(CC) $(CFLAGS) $(OFLAG) zmtx.o zmdm.o crctab.o -o zmtx

zmrx:	zmrx.o zmdm.o crctab.o
	$(CC) $(CFLAGS) $(OFLAG) zmrx.o zmdm.o crctab.o -o zmrx

zmtx.o:		zmtx.c
zmrx.o:		zmrx.c

zmdm.o:		zmdm.c
crctab.o:	crctab.c

clean:
	rm *.o
	rm zmtx zmrx


