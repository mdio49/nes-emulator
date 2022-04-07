CC = gcc
CFLAGS = -g -Wall $(foreach d, $(INCLUDE), -I $d)
TARGET = emu
MAIN = main.o test.o
INCLUDE = emu/include sys/include
OBJECTS = $(filter-out $(MAIN), $(wildcard *.o))

main: main.o
	$(CC) $(CFLAGS) $(OBJECTS) main.o -o $(TARGET).exe

test: test.o
	$(CC) $(CFLAGS) $(OBJECTS) test.o -o $(TARGET)_test.exe 

test.o: emu/src/test.c main.o
	$(CC) $(CFLAGS) -c emu/src/test.c

main.o: emu/src/main.c cpu.o prog.o
	$(CC) $(CFLAGS) -c emu/src/main.c 

gdevice.o: emu/src/gdevice.c glad.o
	$(CC) $(CFLAGS) -c emu/src/shader.c emu/src/gdevice.c

glad.o: emu/src/glad.c
	$(CC) $(CFLAGS) -c emu/src/glad.c

cpu.o: sys/cpu/*.c memory.o
	$(CC) $(CFLAGS) -c sys/cpu/*.c

memory.o: sys/memory/*.c
	$(CC) $(CFLAGS) -c sys/memory/*.c

prog.o: sys/prog/*.c
	$(CC) $(CFLAGS) -c sys/prog/*.c

clean:
	rm *.o *.exe -rf
