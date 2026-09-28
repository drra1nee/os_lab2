CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Isrc

TARGETS = parent.exe child1.exe child2.exe

all: $(TARGETS)

parent.exe: src/parent.c src/utils.c src/common.h
	$(CC) $(CFLAGS) src/parent.c src/utils.c -o parent.exe

child1.exe: src/child.c src/utils.c src/common.h
	$(CC) $(CFLAGS) src/child.c src/utils.c -o child1.exe

child2.exe: src/child.c src/utils.c src/common.h
	$(CC) $(CFLAGS) src/child.c src/utils.c -o child2.exe

clean:
	del /f /q $(TARGETS) 2>nul || rm -f $(TARGETS)

.PHONY: all clean
