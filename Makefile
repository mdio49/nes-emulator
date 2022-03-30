CC = gcc
CFLAGS = -g -Wall -I sys/include
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

cpu.o: sys/cpu/*.c memory.o
	$(CC) $(CFLAGS) -c sys/cpu/*.c 

memory.o: sys/memory/*.c
	$(CC) $(CFLAGS) -c sys/memory/*.c

clean:
	rm *.o *.exe -rf
