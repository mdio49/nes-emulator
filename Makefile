CC = gcc
CFLAGS = -g -Wall
TARGET = emu

main: main.o
	$(CC) $(CFLAGS) *.o -o $(TARGET).exe 

main.o: main.c cpu.o
	$(CC) $(CFLAGS) -c main.c

cpu.o: addr.c cpu.c cpu.h insdefs.c
	$(CC) $(CFLAGS) -c addr.c cpu.c insdefs.c

clean:
	rm *.o $(TARGET).exe -rf
