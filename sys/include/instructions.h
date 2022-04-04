#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <cpu.h>

// Load, store and transfer instructions.

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

// Illegal instructions.

extern const instruction_t INS_ALR;
extern const instruction_t INS_ANC;
extern const instruction_t INS_ANE;
extern const instruction_t INS_ARR;
extern const instruction_t INS_DCP;
extern const instruction_t INS_ISC;
extern const instruction_t INS_LAS;
extern const instruction_t INS_LAX;
extern const instruction_t INS_LXA;
extern const instruction_t INS_RLA;
extern const instruction_t INS_RRA;
extern const instruction_t INS_SAX;
extern const instruction_t INS_SBX;
extern const instruction_t INS_SHA;
extern const instruction_t INS_SHX;
extern const instruction_t INS_SHY;
extern const instruction_t INS_SLO;
extern const instruction_t INS_SRE;
extern const instruction_t INS_TAS;
extern const instruction_t INS_USBC;
extern const instruction_t INS_JAM;


#endif
