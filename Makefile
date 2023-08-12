CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic -g -gdwarf-4
LFLAGS = -lm

.PHONY: all clean

all: httpserver

httpserver: httpserver.o parser.o asgn2_helper_funcs.a
	$(CC) $(CFLAGS) -o httpserver httpserver.o parser.o asgn2_helper_funcs.a $(LFLAGS)

httpserver.o: httpserver.c
	$(CC) $(CFLAGS) -c httpserver.c

parser.o: parser.c
	$(CC) $(CFLAGS) -c parser.c

asgn2_helper_funcs.a: asgn2_helper_funcs.c
	$(CC) $(CFLAGS) -c asgn2_helper_funcs.c

clean:
	rm -f *.o httpserver

clang-format:
	clang-format -i -style=file *.h *.c
