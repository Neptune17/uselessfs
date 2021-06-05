CC=g++
CFLAGS=-g -Wall -D_FILE_OFFSET_BITS=64
LDFLAGS=-lfuse

all: main.cpp
	$(CC) main.cpp $(LDFLAGS) $(CFLAGS) -o uselessfs

clean:
	rm uselessfs