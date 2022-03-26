CC = gcc
CFLAGS = -g -Wall
TARGET = emu

main: main.o
	$(CC) $(CFLAGS) *.o -o $(TARGET).exe 

main.o: main.c cpu.o
	$(CC) $(CFLAGS) -c main.c

cpu.o: addr.c addr.h cpu.c cpu.h instructions.c instructions.h opcodes.c
	$(CC) $(CFLAGS) -c addr.c cpu.c instructions.c opcodes.c

clean:
	rm *.o $(TARGET).exe -rf
