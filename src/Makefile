CC      = cc
CFLAGS  = -Wall -Wextra -std=c11 -O2
PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin

TARGETS = glosslint glossemit

all: $(TARGETS)

glosslint: glosslint.o
	$(CC) $(CFLAGS) -o $@ $^

glossemit: glossemit.o
	$(CC) $(CFLAGS) -o $@ $^ -ljansson

glosslint.o: glosslint.c glosslint.h
	$(CC) $(CFLAGS) -c glosslint.c

glossemit.o: glossemit.c
	$(CC) $(CFLAGS) -c glossemit.c

install: $(TARGETS)
	mkdir -p $(BINDIR)
	cp $(TARGETS) $(BINDIR)/

clean:
	rm -f $(TARGETS) *.o

.PHONY: all install clean
