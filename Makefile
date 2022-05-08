# Compiler and target.
CC = gcc
TARGET = emu

# Paths.
INC_PATHS = emu/include SDL2/include sys/include
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

test.o: emu/test/*.c cpu.o
	$(CC) $(CFLAGS) -c emu/test/*.c

main.o: emu/src/*.c sys.o
	$(CC) $(CFLAGS) -c emu/src/*.c

sys.o: apu.o cpu.o ppu.o prog.o
	$(CC) $(CFLAGS) -c sys/nes/*.c

prog.o: sys/prog/*.c mappers.o
	$(CC) $(CFLAGS) -c sys/prog/*.c

apu.o: sys/apu/*.c cpu.o
	$(CC) $(CFLAGS) -c sys/apu/*.c

cpu.o: sys/cpu/*.c memory.o
	$(CC) $(CFLAGS) -c sys/cpu/*.c

ppu.o: sys/ppu/*.c memory.o
	$(CC) $(CFLAGS) -c sys/ppu/*.c

mappers.o: sys/mappers/*.c memory.o
	$(CC) $(CFLAGS) -c sys/mappers/*.c

memory.o: sys/memory/*.c
	$(CC) $(CFLAGS) -c sys/memory/*.c

clean:
	rm *.o *.exe -rf
