# compiler
CC = g++

# compiler options
CFLAGS = -c -Wall

all: tester

tester: tester.o
	$(CC) -lm tester.o -o ProgramTester

tester.o: ProgramTester.cpp
	$(CC) $(CFLAGS) ProgramTester.cpp

clean:
	rm -rf *o tester
