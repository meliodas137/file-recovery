CC=gcc -pthread
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra
LDFLAGS=-lcrypto

.PHONY: all
all: nyufile

nyufile: nyufile.o

nyufile.o: nyufile.c fatstruct.h

.PHONY: clean
clean:
	rm -f *.o nyufile
