CC=gcc
CFLAGS = -c -g -Wall -Werror

all: FS

FS: io/Controller.o apps/test01.o io/File.o
	$(CC) io/Controller.o apps/test01.o io/File.o -o FS

Controller.o: io/Controller.c
	$(CC) $(CFLAGS) io/Controller.c

test01.o: apps/test01.c
	$(CC) $(CFLAGS) apps/test01.c

#test02.o: apps/test02.c
#	$(CC) $(CFLAGS) apps/test02.c

File.o: io/File.c
	$(CC) $C(FLAGS) io/File.c

clean:
	rm -rf io/*.o apps/*.o *.o FS
