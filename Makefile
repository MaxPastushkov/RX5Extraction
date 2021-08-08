CC=gcc

CFLAGS=-Wall -O2

.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) -o $*.o -c $*.c

all: header-gen extractor

header-gen: header-gen.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ header-gen.o

header-gen.o: header-gen.c

extractor: extractor.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ extractor.o

clean:
	rm -f header-gen.o header-gen extractor.o extractor *.wav *.pcm
