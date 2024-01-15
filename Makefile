CC=gcc
CFLAGS=-Wall -lpthread -lsqlite3

all: sqlite-thread

sqlite-thread: sqlite-thread.c
	$(CC) sqlite-thread.c -o sqlite-thread $(CFLAGS)

clean:
	rm -rf sqlite-thread
