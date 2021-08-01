CC=gcc

CFLAGS=-Wall -O2
OFILES=header-gen.o

.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) -o $*.o -c $*.c

header-gen: $(OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OFILES)

header-gen.o: header-gen.c

clean:
	rm -f $(OFILES) header-gen *.wav
