OPTLEVEL=3
CFLAGS=-g -Wall -flto -O${OPTLEVEL}
LDFLAGS=-flto

.PHONY: clean

bm: main.o bm.o

main.o: main.c bm.h

bm.o: bm.c bm.h

clean:
	-$(RM) bm *.o



