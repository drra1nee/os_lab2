CC = gcc
CFLAGS = -Wall -Wextra -std=c11

TARGETS = parent.exe child1.exe child2.exe

all: $(TARGETS)

parent.exe: parent.c utils.c common.h
	$(CC) $(CFLAGS) parent.c utils.c -o parent.exe

child1.exe: child.c utils.c common.h
	$(CC) $(CFLAGS) child.c utils.c -o child1.exe

child2.exe: child.c utils.c common.h
	$(CC) $(CFLAGS) child.c utils.c -o child2.exe

clean:
	del /f /q $(TARGETS) 2>nul || rm -f $(TARGETS)

.PHONY: all clean
