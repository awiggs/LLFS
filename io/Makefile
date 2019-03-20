CC = gcc
CFLAGS = -Wall -Werror -c -o

LLFS: File.o
	$(CC) -o LLFS File.o

File.o: File.c LLFS.h
	$(CC) $(CFLAGS) File.o File.c

clean:
	rm -rf *.o llfs
