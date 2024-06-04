.PHONY: all clean

all: myshell

myshell: myshell.o linkedlist.o
	gcc -o myshell myshell.o linkedlist.o


myshell.o: myshell.c linkedlist.h
	gcc -c myshell.c


linkedlist.o: linkedlist.c
	gcc -c linkedlist.c

clean:
	rm -rf *.o myshell
