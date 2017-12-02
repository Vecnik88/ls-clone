all: clean ls-clone

CC = gcc
SRC = ./src
CFLAGS =-g -Wall -g3 -O0
SOURCES = $(wildcard ./src/*.c)
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = ls-clone
INSTPATH = /usr/bin

.PHONY: all clean run install uninstall

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	-rm -f ./src/*.o $(EXECUTABLE)
	-rm -f ls-clone

install:
	sudo install $(EXECUTABLE) $(INSTPATH)

uninstall:
	sudo rm $(INSTPATH)/$(EXECUTABLE)

run:
	$(INSTPATH)/$(EXECUTABLE)