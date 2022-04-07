CC = gcc
CFLAGS = -g -Wall $(foreach d, $(INCLUDE), -I $d)
TARGET = emu
MAIN = main.o test.o
INCLUDE = emu/include sys/include
OBJECTS = $(filter-out $(MAIN), $(wildcard *.o))

main: main.o
	$(CC) $(CFLAGS) $(OBJECTS) main.o -o $(TARGET).exe -L emu/bin/*.a

test: test.o
	$(CC) $(CFLAGS) $(OBJECTS) test.o -o $(TARGET)_test.exe 

main.o: emu/main.c cpu.o prog.o
	$(CC) $(CFLAGS) -c emu/main.c

test.o: test.c main.o
	$(CC) $(CFLAGS) -c emu/test.c

cpu.o: sys/cpu/*.c memory.o
	$(CC) $(CFLAGS) -c sys/cpu/*.c

memory.o: sys/memory/*.c
	$(CC) $(CFLAGS) -c sys/memory/*.c

prog.o: sys/prog/*.c
	$(CC) $(CFLAGS) -c sys/prog/*.c

clean:
	rm *.o *.exe -rf
