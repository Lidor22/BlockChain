CC=gcc -g

LinkLibraries= -lz -lpthread

BITCOIN_FILE=bitcoin.c

BITCOIN_OUT=-o bitcoin.out

PROGS=bitcoin
 

all: $(PROGS)

bitcoin: 
	$(CC) $(BITCOIN_FILE) $(BITCOIN_OUT) $(LinkLibraries)


clean:
	rm *.o *.gch $(PROGS) 
