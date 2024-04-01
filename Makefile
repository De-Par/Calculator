CC = gcc
CCFLAGS = -g3 -Wall -O3
O_FILES = calc.o

all: calc.c header.h 
	$(CC) $(CCFLAGS) -o app calc.c -lm

clean:
	rm -f app.exe
	rm -f app
	rm -f *.o
