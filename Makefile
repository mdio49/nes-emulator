CC = gcc
CFLAGS = -g -Wall
TARGET = emu
MAIN = main.o test.o
OBJECTS = $(filter-out $(MAIN), $(wildcard *.o))

main: main.o
	$(CC) $(CFLAGS) $(OBJECTS) main.o -o $(TARGET).exe 

test: test.o
	$(CC) $(CFLAGS) $(OBJECTS) test.o -o $(TARGET)_test.exe 

test.o: test.c main.o
	$(CC) $(CFLAGS) -c test.c

main.o: main.c cpu.o
	$(CC) $(CFLAGS) -c main.c

cpu.o: addr.c addr.h cpu.c cpu.h instructions.c instructions.h opcodes.c
	$(CC) $(CFLAGS) -c addr.c cpu.c instructions.c opcodes.c

clean:
	rm *.o $(TARGET).exe -rf
