CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99
VALGRIND = valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 -q

.PHONY: all clean tests hello

all: hello

hello: hello.c realm.h
	$(CC) $(CFLAGS) -o hello.exe hello.c
	./hello.exe

tests: tst/unit.c tst/app.c tst/bench.c realm.h
	$(CC) $(CFLAGS) -I. -o tst/unit.exe tst/unit.c
	$(VALGRIND) ./tst/unit.exe
	$(CC) $(CFLAGS) -I. -o tst/app.exe tst/app.c
	$(VALGRIND) ./tst/app.exe
	$(CC) $(CFLAGS) -I. -o tst/bench.exe tst/bench.c
	$(VALGRIND) ./tst/bench.exe

clean:
	rm -f hello.exe tst/unit.exe tst/app.exe tst/bench.exe
