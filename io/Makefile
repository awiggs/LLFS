CC = gcc
CFLAGS = -Wall -Werror -c -o

LLFS: File.o Controller.o
	$(CC) -o LLFS File.o Controller.o

File.o: File.c
	$(CC) $(CFLAGS) File.o File.c

Controller.o: Controller.c
	$(CC) $(CFLAGS) Controller.o Controller.c

clean:
	rm -rf *.o llfs
