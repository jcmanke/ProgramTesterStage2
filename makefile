# compiler
CC = g++

# compiler options
CFLAGS = -c

#OBJECT FILES
OBJS = ProgramTester.cpp

tester: ProgramTester.cpp
	${CC} -lm ${OBJS} -o tester
