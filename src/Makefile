CC      = cc
CFLAGS  = -Wall -Wextra -std=c11 -O2
PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin

TARGET  = glosslint
OBJS    = glosslint.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

glosslint.o: glosslint.c glosslint.h
	$(CC) $(CFLAGS) -c glosslint.c

install: $(TARGET)
	mkdir -p $(BINDIR)
	cp $(TARGET) $(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all install clean
