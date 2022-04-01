#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include "vm.h"

#define PAGE_MASK       0x00FF
#define PAGE_SIZE       0x0100
#define WMEM_SIZE       0x0800

#define AS_CPU          0x0000
#define AS_IO           0x2000
#define AS_EXP          0x4020
#define AS_SAVE         0x6000
#define AS_PROG         0x8000

#define STACK_START     0x0100

#define SR_CARRY        0x01
#define SR_ZERO         0x02
#define SR_INTERRUPT    0x04
#define SR_DECIMAL      0x08
#define SR_BREAK        0x10
#define SR_IGNORED      0x20
#define SR_OVERFLOW     0x40
#define SR_NEGATIVE     0x80

/**
 * @brief The flags for each bit in the status register.
 */
typedef union sr_flags {
    struct flags {
        unsigned carry : 1;     // Carry flag.
        unsigned zero  : 1;     // Zero flag.
        unsigned irq   : 1;     // Interrupt flag (IRQ disable).
        unsigned dec   : 1;     // Decimal flag.
        unsigned brk   : 1;     // Break flag.
        unsigned ign   : 1;     // Ignored flag (unused).
        unsigned vflow : 1;     // Overflow flag.
        unsigned neg   : 1;     // Negative/sign flag.
    } flags;
    uint8_t bits;
} srflags_t;

/**
 * @brief A trap frame that stores the state of the CPU's registers.
 */
typedef struct tframe {
    uint16_t    pc;     // program counter
    uint8_t     ac;     // accumulator
    uint8_t     x;      // X register
    uint8_t     y;      // Y register
    srflags_t   sr;     // status register
    uint8_t     sp;     // stack pointer
} tframe_t;

/**
 * @brief A CPU struct that contains all data needed to emulate the CPU.
 */
typedef struct cpu {

    tframe_t        frame;      // The CPU's registers.
    addrspace_t     *as;        // The CPU's address space.
    uint8_t         *wmem;      // The CPU's working memory.
    uint8_t         *stack;     // A pointer to the bottom of the stack (increment by stack pointer register to get current value in stack).

} cpu_t;

/**
 * @brief An addressing mode.
 */
typedef struct addrmode {

    /**
    * @brief Gets a pointer to the value of the argument using this address mode.
    */
    const vaddr_ptr_pair_t (*evaluate)(const tframe_t *frame, const addrspace_t *as, const uint8_t *args);

    /**
    * @brief The number of operator arguments (bytes) required by this address mode in
    * order to obtain the address. Upon execution of an instruction, the program counter
    * is incremented by 1 + this value.
    */
    const uint8_t argc;

} addrmode_t;

typedef struct instruction {

    /**
     * @brief The name of the instruction.
     */
    const char *name;

    /**
     * @brief Applies the instruction to the given address after it has been evaluated by its addressing mode.
     */
    void (*apply)(tframe_t *frame, const addrspace_t *as, addr_t addr, uint8_t *value);

    /**
     * @brief Indicates whether the instruction is a jump instruction, in which case the program counter shouldn't increment after executing.
     */
    const bool jump;

} instruction_t;

/**
 * @brief Opcodes are formatted in a pattern a-b-c where a and b are 3-bit numbers
 * and c is a 2-bit number. This organises the instructions so that the type of
 * instruction and address mode can be extracted more easily.
 */
typedef struct opcode {
    
    unsigned group    : 2;      // The least significant 2-bits of the opcode (determines which group the instruction falls under).
    unsigned addrmode : 3;      // The middle 3-bits of the optcode (often determines the address mode).
    unsigned num      : 3;      // The most significant 3-bits of the opcode (often determines the instruction).

} opcode_t;

/**
 * @brief A union to convert raw byte values into opcodes and vice versa.
 */
typedef union opcode_converter {
    opcode_t    opcode;
    uint8_t     raw;
} opcode_converter_t;

/**
 * @brief A single CPU operation that contains an instruction along with the
 * address mode that will be used to process the argument(s) to this instruction.
 */
typedef struct operation {

    const addrmode_t      *addr_mode;     // The address mode to use.
    const instruction_t   *instruction;   // The instruction to execute.
    const uint8_t         *args;          // The arguments of the instruction.

} operation_t;

/**
 * @brief Determines the address mode for the given opcode.
 * 
 * @param opc The opcode.
 * @return The address mode.
 */
const addrmode_t *get_address_mode(opcode_t opc);

/**
 * @brief Determines the instruction for the given opcode.
 * 
 * @param opc The opcode.
 * @return The instruction.
 */
const instruction_t *get_instruction(opcode_t opc);

/**
 * @brief Converts two bytes into a 16-bit word.
 * 
 * @param low The least significant byte.
 * @param high The most significant byte.
 * @return The resultant 16-bit word.
 */
uint16_t bytes_to_word(uint8_t low, uint8_t high);

/**
 * @brief Creates a new instance of an emulated CPU.
 * 
 * @return The CPU struct that holds the CPU data.
 */
cpu_t *cpu_create(void);

/**
 * @brief Frees the memory associated with the given CPU, including the underlying address space.
 * 
 * @param cpu The CPU to destroy.
 */
void cpu_destroy(cpu_t *cpu);

/**
 * @brief Fetches the next instruction from memory without advancing the program counter.
 * 
 * @param cpu The CPU's state.
 * @return A pointer to the next instruction.
 */
uint8_t *cpu_fetch(const cpu_t *cpu);

/**
 * @brief Decodes the given instruction.
 * 
 * @param insptr The instruction pointer.
 * @return A struct containing the decoded instruction.
 */
operation_t cpu_decode(const uint8_t *insptr);

/**
 * @brief Executes an instruction. Advances the program counter after the instruction has been executed.
 * 
 * @param cpu The CPU's state.
 * @param op The instruction to execute.
 */
void cpu_execute(cpu_t *cpu, operation_t op);

#endif
