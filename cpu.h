#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define PAGE_MASK       0x00FF
#define PAGE_SIZE       0x0100
#define MEM_SIZE        0xFFFF

#define AS_CPU          0x0000
#define AS_IO           0x2000
#define AS_EXP          0x4020
#define AS_SAVE         0x6000
#define AS_PROG         0x8000

#define SR_NEGATIVE     0x80
#define SR_OVERFLOW     0x40
#define SR_IGNORED      0x20
#define SR_BREAK        0x10
#define SR_DECIMAL      0x08
#define SR_INTERRUPT    0x04
#define SR_ZERO         0x02
#define SR_CARRY        0x01

typedef struct sr_flags {
    unsigned flag_negative  : 1;
    unsigned flag_overflow  : 1;
    unsigned flag_ignored   : 1;
    unsigned flag_break     : 1;
    unsigned flag_decimal   : 1;
    unsigned flag_interrupt : 1;
    unsigned flag_zero      : 1;
    unsigned flag_carry     : 1;
} srflags_t;

/**
 * A trap frame that stores the state of the CPU's registers.
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
 * Memory addresses are 16-bit ranging from 0x00 to 0xFF.
 */
typedef uint16_t addr_t;

/**
 * An addressing mode.
 */
typedef struct addrmode {

    /**
    * Gets the absolute address of the argument using this address mode.
    */
    addr_t (*get_address)(const tframe_t *frame, const uint8_t *mem, const uint8_t *args);

    /**
    * The number of operator arguments (bytes) required by this address mode
    * in order to obtain the address. Upon execution of an instruction, the
    * program counter is incremented by 1 + this value.
    */
    uint8_t argc;

} addrmode_t;

/**
 * There are a wide range of addresisng modes, and not all instructions use every one.
 * 
 * IMPLIED:                 Argument is implied (i.e. instruction takes no arguments).
 * ACCUMULATOR: A           Argument is value from accumulator (instruction takes no arguments).
 * IMMEDIATE:   #$BB        Instruction uses the constant value provided. If an instruction uses this address mode,
 *                          then the pointer to the address mode type is NULL.
 * 
 * ZEROPAGE:    $LL         Zero-page addressing. Instruction applies to the memory location $00LL.
 * ZEROPAGEX:   $LL,X       Same as zero page, but offset by the value at register X. Wraps around if larger than 255
 *                          so that memory address is always between $0000 and $00FF.
 * ZEROPAGEY:   $LL,Y       Same as zero page X, but for the register Y.
 * 
 * ABSOLUTE:    $LLHH       Absolute addressing. Instruction appllies to the memory location $HHLL.
 * ABSOLUTEX:   $LLHH,X.    Same as absolute, but offset by the value at register X.
 * ABSOLUTEY:   $LLHH,Y.    Same as absolute X, but for the register Y.
 * 
 * RELATIVE:    $BB         Relative addressing. Target address is program counter offset by the signed value $BB.
 * 
 * INDIRECT:    ($LLHH)     Indirect addressing. Similar to a pointer. Instruction takes the value at memory addresses
 *                          $HHLL and $HHLL+1 and uses that to form a new memory address. The value at the first address
 *                          gives the least significant byte for the new address and the value at the second address
 *                          gives the most significant byte, so the new address is ($HHLL+1, $HHLL).
 * INDIRECTX:   ($LL,X)     Indexed-indirect addressing. Takes the zero-page address $00LL, increments it by the value
 *                          at register X wrapped between 0-255, then use it to lookup a 2-byte memory address.
 *                          ~ Basically a composition of zeropage-x and indirect.
 * INDIRECTY:   ($LL),Y     Indirect-indexed addressing. Take the zero-page address $LL, dereference it to form a 2-byte
 *                          memory address, then add the value at register Y to form the final memory address.
 *                          ~ Basically a composition of zeropage, indirect and y-indexed absolute.
 */

extern const addrmode_t AM_IMPLIED;
extern const addrmode_t AM_ACCUMULATOR;

extern const addrmode_t AM_ZEROPAGE;
extern const addrmode_t AM_ZEROPAGE_X;
extern const addrmode_t AM_ZEROPAGE_Y;

extern const addrmode_t AM_ABSOLUTE;
extern const addrmode_t AM_ABSOLUTE_X;
extern const addrmode_t AM_ABSOLUTE_Y;

extern const addrmode_t AM_RELATIVE;

extern const addrmode_t AM_INDIRECT;
extern const addrmode_t AM_INDIRECT_X;
extern const addrmode_t AM_INDIRECT_Y;

typedef struct instruction {

    /**
     * The name of the instruction.
     */
    const char *name;

    /**
     * Applies the instruction to the given address after it has been evaluated by its addressing mode.
     */
    void (*apply)(tframe_t *frame, uint8_t *mem, addr_t addr);
    
    /**
     * Applies the instruction in immediate addressing mode using the given value. Not all instructions
     * support immediate addressing, in which case this will be NULL.
     */
    void (*apply_immediate)(tframe_t *frame, uint8_t *mem, uint8_t value);

} instruction_t;

extern const instruction_t INS_BRK;
extern const instruction_t INS_JSR;

// Load, store and interregister instructions.

extern const instruction_t INS_LDA;
extern const instruction_t INS_LDX;
extern const instruction_t INS_LDY;

extern const instruction_t INS_STA;
extern const instruction_t INS_STX;
extern const instruction_t INS_STY;

extern const instruction_t INS_TAX;
extern const instruction_t INS_TAY;
extern const instruction_t INS_TSX;
extern const instruction_t INS_TXA;
extern const instruction_t INS_TXS;
extern const instruction_t INS_TYA;

// Stack instructions.

extern const instruction_t INS_PHA;
extern const instruction_t INS_PHP;
extern const instruction_t INS_PLA;
extern const instruction_t INS_PLP;

// Decrements and increments.

extern const instruction_t INS_DEC;
extern const instruction_t INS_DEX;
extern const instruction_t INS_DEY;
extern const instruction_t INS_INC;
extern const instruction_t INS_INX;
extern const instruction_t INS_INY;

// Arithmetic operators.

extern const instruction_t INS_ADC;
extern const instruction_t INS_SBC;

// Logical operators.

extern const instruction_t INS_AND;
extern const instruction_t INS_EOR;
extern const instruction_t INS_ORA;

// Shift & rotate instructions.

extern const instruction_t INS_ASL;
extern const instruction_t INS_LSR;
extern const instruction_t INS_ROL;
extern const instruction_t INS_ROR;

// Flag instructions.

extern const instruction_t INS_CLC;
extern const instruction_t INS_CLD;
extern const instruction_t INS_CLI;
extern const instruction_t INS_CLV;
extern const instruction_t INS_SEC;
extern const instruction_t INS_SED;
extern const instruction_t INS_SEI;

// Comparisons.

extern const instruction_t INS_CMP;
extern const instruction_t INS_CPX;
extern const instruction_t INS_CPY;

// Conditional branch instructions.

extern const instruction_t INS_BCC;
extern const instruction_t INS_BCS;
extern const instruction_t INS_BEQ;
extern const instruction_t INS_BMI;
extern const instruction_t INS_BNE;
extern const instruction_t INS_BPL;
extern const instruction_t INS_BVC;
extern const instruction_t INS_BVS;

// Jumps and subroutines.

extern const instruction_t INS_JMP;
extern const instruction_t INS_JSR;
extern const instruction_t INS_RTS;

// Interrupts.

extern const instruction_t INS_BRK;
extern const instruction_t INS_RTI;

// Other.

extern const instruction_t INS_BIT;
extern const instruction_t INS_NOP;

/**
 * Opcodes are formatted in a pattern a-b-c where a and b are 3-bit numbers
 * and c is a 2-bit number. This organises the instructions so that the type of
 * instruction and address mode can be extracted more easily.
 */
typedef struct opcode {
    
    unsigned group    : 2;      // The final 2-bits of the opcode (determines which group the instruction falls under).
    unsigned addrmode : 3;      // The second 3-bits of the optcode (often determines the address mode).
    unsigned num      : 3;      // The first 3-bits of the opcode (often determines the instruction).

} opcode_t;

/**
 * A single CPU operation that contains an instruction along with the address mode
 * that will be used to process the argument(s) to this instruction.
 */
typedef struct operation {

    const instruction_t   *instruction;   // The instruction to execute.
    const addrmode_t      *addr_mode;     // The address mode to use.
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
 * @brief Fetches the next instruction from memory without advancing the program counter.
 * 
 * @param frame The CPU state.
 * @param mem The memory.
 * @return A pointer to the next instruction.
 */
uint8_t *fetch(const tframe_t *frame, uint8_t *mem);

/**
 * @brief Decodes the given instruction.
 * 
 * @param insptr The instruction pointer.
 * @return A struct containing the decoded instruction.
 */
operation_t decode(const uint8_t *insptr);

/**
 * @brief Executes an instruction. Advances the program counter after the instruction has been executed.
 * 
 * @param frame The CPU state.
 * @param mem The memory.
 * @param op The instruction to execute.
 */
void execute(tframe_t *frame, uint8_t *mem, operation_t op);

#endif
