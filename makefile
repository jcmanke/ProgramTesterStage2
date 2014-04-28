# compiler
CC = g++

# compiler options
CFLAGS = -c

#OBJECT FILES
OBJS = ProgramTester.cpp

tester: ProgramTester.cpp

	${CC} -g -lm ${OBJS} -o test
