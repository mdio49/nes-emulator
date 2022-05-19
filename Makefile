# Compiler and target.
CC = gcc
TARGET = emu

# Paths.
INC_PATHS = emu/include SDL2/include sys/include
LIB_PATHS = SDL2/lib
OBJ_PATH = bin

# Flags
LIB_FLAGS = $(foreach d, $(LIB_PATHS),-L $d)
INC_FLAGS = $(foreach d, $(INC_PATHS),-I $d)
CFLAGS = -g -Wall $(INC_FLAGS)

# Linker.
LINKER_INPUT = SDL2 SDL2main
LINKER_FLAGS = $(foreach d, $(LINKER_INPUT),-l $d)

# Header files.
APU_H = sys/include/apu.h
CPU_H = sys/include/addrmodes.h sys/include/cpu.h sys/include/instructions.h
EMU_H = emu/include/*.h
MAPPERS_H = sys/include/mapper.h sys/include/mappers.h
MEMORY_H = sys/include/vm.h
PPU_H = sys/include/color.h sys/include/ppu.h
PROG_H = sys/include/ines.h sys/include/prog.h
SYS_H = sys/include/sys.h

# Source directories.
APU_DIR = sys/apu
CPU_DIR = sys/cpu
MAPPERS_DIR = sys/mappers
MEMORY_DIR = sys/memory
PPU_DIR = sys/ppu
PROG_DIR = sys/prog
SYS_DIR = sys/nes

EMU_DIR = emu/src
TEST_DIR = emu/test

# Object files.
APU = $(patsubst $(APU_DIR)/%.c,$(OBJ_PATH)/%.o, $(wildcard $(APU_DIR)/*.c))
CPU = $(patsubst $(CPU_DIR)/%.c,$(OBJ_PATH)/%.o, $(wildcard $(CPU_DIR)/*.c))
MAPPERS = $(patsubst $(MAPPERS_DIR)/%.c,$(OBJ_PATH)/%.o, $(wildcard $(MAPPERS_DIR)/*.c))
MEMORY = $(patsubst $(MEMORY_DIR)/%.c,$(OBJ_PATH)/%.o, $(wildcard $(MEMORY_DIR)/*.c))
PPU = $(patsubst $(PPU_DIR)/%.c,$(OBJ_PATH)/%.o, $(wildcard $(PPU_DIR)/*.c))
PROG = $(patsubst $(PROG_DIR)/%.c,$(OBJ_PATH)/%.o, $(wildcard $(PROG_DIR)/*.c))
SYS = $(patsubst $(SYS_DIR)/%.c,$(OBJ_PATH)/%.o, $(wildcard $(SYS_DIR)/*.c))

ALL = $(APU) $(CPU) $(MAPPERS) $(MEMORY) $(PPU) $(PROG) $(SYS)
EMU = $(patsubst $(EMU_DIR)/%.c,$(OBJ_PATH)/%.o, $(wildcard $(EMU_DIR)/*.c))
TEST = $(patsubst $(TEST_DIR)/%.c,$(OBJ_PATH)/%.o, $(wildcard $(TEST_DIR)/*.c))

# Main targets.

main: $(EMU)
	$(CC) $(CFLAGS) $(EMU) $(ALL) -o $(TARGET) $(LIB_FLAGS) $(LINKER_FLAGS)

test: $(TEST)
	$(CC) $(CFLAGS) $(TEST) $(CPU) $(MEMORY) -o $(TARGET)_test

clean:
	rm $(OBJ_PATH)/*.o *.exe -rf

# Dependency targets.

$(EMU): $(OBJ_PATH)/%.o: $(EMU_DIR)/%.c $(EMU_H) $(SYS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST): $(OBJ_PATH)/%.o: $(TEST_DIR)/%.c $(CPU)
	$(CC) $(CFLAGS) -c $< -o $@

$(SYS): $(OBJ_PATH)/%.o: $(SYS_DIR)/%.c $(SYS_H) $(APU) $(CPU) $(PPU) $(PROG)
	$(CC) $(CFLAGS) -c $< -o $@

$(PROG): $(OBJ_PATH)/%.o: $(PROG_DIR)/%.c $(PROG_H) $(MAPPERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(APU): $(OBJ_PATH)/%.o: $(APU_DIR)/%.c $(API_H) $(CPU)
	$(CC) $(CFLAGS) -c $< -o $@

$(CPU): $(OBJ_PATH)/%.o: $(CPU_DIR)/%.c $(CPU_H) $(MEMORY) 
	$(CC) $(CFLAGS) -c $< -o $@

$(PPU): $(OBJ_PATH)/%.o: $(PPU_DIR)/%.c $(PPU_H) $(MEMORY) 
	$(CC) $(CFLAGS) -c $< -o $@

$(MAPPERS): $(OBJ_PATH)/%.o: $(MAPPERS_DIR)/%.c $(MAPPERS_H) $(MEMORY)
	$(CC) $(CFLAGS) -c $< -o $@

$(MEMORY): $(OBJ_PATH)/%.o: $(MEMORY_DIR)/%.c $(VM_H)
	$(CC) $(CFLAGS) -c $< -o $@
