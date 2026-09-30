CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -Iinclude

SRC = src/hashtable.c
TEST = tests/main.c

all: bin/test bin/libhash.so 

bin/test: $(SRC) $(TEST) include/hashtable.h | bin
	$(CC) $(CFLAGS) $(SRC) $(TEST) -o bin/test

bin/libhash.so: $(SRC) include/hashtable.h | bin
	$(CC) $(CFLAGS) -fPIC -shared $(SRC) -o bin/libhash.so

bin:
	mkdir -p bin

test: bin/test
	./bin/test

memcheck: bin/test
	valgrind --leak-check=full ./bin/test

clean:
	rm -f bin/test bin/libhash.so

.PHONY: all test memcheck clean
