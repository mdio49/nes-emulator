# Compiler and target.
CC = gcc
TARGET = emu

# Paths.
INC_PATHS = sys/include SDL2/include
LIB_PATHS = SDL2/lib
SRC_PATH = .

# Flags
LIB_FLAGS = $(foreach d, $(LIB_PATHS),-L $d)
INC_FLAGS = $(foreach d, $(INC_PATHS),-I $d)
CFLAGS = -g -Wall $(INC_FLAGS)

# Linker.
LINKER_INPUT = SDL2 SDL2main
LINKER_FLAGS = $(foreach d, $(LINKER_INPUT),-l $d)

# Object files (excluding main).
MAIN = main.o test.o
OBJECTS = $(filter-out $(MAIN), $(wildcard *.o))

main: main.o
	$(CC) $(CFLAGS) $(OBJECTS) main.o -o $(TARGET) $(LIB_FLAGS) $(LINKER_FLAGS)

test: test.o
	$(CC) $(CFLAGS) $(OBJECTS) test.o -o $(TARGET)_test 

test.o: test.c
	$(CC) $(CFLAGS) -c test.c

main.o: main.c sys.o
	$(CC) $(CFLAGS) -c main.c

sys.o: cpu.o ppu.o prog.o
	$(CC) $(CFLAGS) -c sys/nes/*.c

cpu.o: sys/cpu/*.c memory.o
	$(CC) $(CFLAGS) -c sys/cpu/*.c

ppu.o: sys/ppu/*.c memory.o
	$(CC) $(CFLAGS) -c sys/ppu/*.c

memory.o: sys/memory/*.c
	$(CC) $(CFLAGS) -c sys/memory/*.c

prog.o: sys/prog/*.c
	$(CC) $(CFLAGS) -c sys/prog/*.c

clean:
	rm *.o *.exe -rf
