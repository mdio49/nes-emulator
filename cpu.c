#include <stdio.h>
#include <stdlib.h>

#include "addr.h"
#include "cpu.h"
#include "instructions.h"

typedef union opcode_converter {
    opcode_t    opcode;
    uint8_t     raw;
} opcode_converter_t;

const instruction_t *INS_GROUP1[8][8] = {
    { &INS_BRK, &INS_NOP, &INS_PHP, &INS_NOP, &INS_BPL, &INS_NOP, &INS_CLC, &INS_NOP },
    { &INS_JSR, &INS_BIT, &INS_PLP, &INS_BIT, &INS_BMI, &INS_NOP, &INS_SEC, &INS_NOP },
    { &INS_RTI, &INS_NOP, &INS_PHA, &INS_JMP, &INS_BVC, &INS_NOP, &INS_CLI, &INS_NOP },
    { &INS_RTS, &INS_NOP, &INS_PLA, &INS_JMP, &INS_BVS, &INS_NOP, &INS_SEI, &INS_NOP },
    { &INS_NOP, &INS_STY, &INS_DEY, &INS_STY, &INS_BCC, &INS_STY, &INS_TYA, NULL     },
    { &INS_LDY, &INS_LDY, &INS_TAY, &INS_LDY, &INS_BCS, &INS_LDY, &INS_CLV, &INS_LDY },
    { &INS_CPY, &INS_CPY, &INS_INY, &INS_CPY, &INS_BNE, &INS_NOP, &INS_CLD, &INS_NOP },
    { &INS_CPX, &INS_CPX, &INS_INX, &INS_CPX, &INS_BEQ, &INS_NOP, &INS_SED, &INS_NOP }
};

const instruction_t *INS_GROUP2[8] = {
	&INS_ORA,
	&INS_AND,
	&INS_EOR,
	&INS_ADC,
	&INS_STA,
	&INS_LDA,
	&INS_CMP,
	&INS_SBC
};

const instruction_t *INS_GROUP3[8][8] = {
    { NULL    , &INS_ASL, &INS_ASL, &INS_ASL, NULL    , &INS_ASL, &INS_NOP, &INS_ASL },
    { NULL    , &INS_ROL, &INS_ROL, &INS_ROL, NULL    , &INS_ROL, &INS_NOP, &INS_ROL },
    { NULL    , &INS_LSR, &INS_LSR, &INS_LSR, NULL    , &INS_LSR, &INS_NOP, &INS_LSR },
    { NULL    , &INS_ROR, &INS_ROR, &INS_ROR, NULL    , &INS_ROR, &INS_NOP, &INS_ROR },
    { &INS_NOP, &INS_STX, &INS_TXA, &INS_STX, NULL    , &INS_STX, &INS_TXS, NULL     },
    { &INS_LDX, &INS_LDX, &INS_TAX, &INS_LDX, NULL    , &INS_LDX, &INS_TSX, &INS_LDX },
    { &INS_NOP, &INS_DEC, &INS_DEX, &INS_DEC, NULL    , &INS_DEC, &INS_NOP, &INS_DEC },
    { &INS_NOP, &INS_INC, &INS_NOP, &INS_INC, NULL    , &INS_INC, &INS_NOP, &INS_INC }
};

const addrmode_t *get_address_mode(opcode_t opc) {
    const addrmode_t *am;
    switch (opc.addrmode) {
        case 0x00:
            if (opc.group == 0x00) {
                if (opc.num == 0x01) {
                    am = &AM_ABSOLUTE;
                }
                else if (opc.num <= 0x03) {
                    am = &AM_IMPLIED;
                }
                else {
                    am = &AM_IMMEDIATE;
                }
            }
            else if (opc.group == 0x02) {
                am = &AM_IMMEDIATE;
            }
            else {
                am = &AM_INDIRECT;
            }
            break;
        case 0x01:
            am = &AM_ZEROPAGE;
            break;
        case 0x02:
            if (opc.group == 0x00) {
                am = &AM_IMPLIED;
            }
            else if (opc.group == 0x02) {
                am = opc.num <= 0x03 ?  &AM_ACCUMULATOR : &AM_IMPLIED;
            }
            else {
                am = &AM_IMMEDIATE;
            }
            break;
        case 0x03:
            am = (opc.group == 0x00 && opc.num == 0x03) ? &AM_INDIRECT : &AM_ABSOLUTE;
            break;
        case 0x04:
            am = (opc.group == 0x00) ? &AM_RELATIVE : &AM_INDIRECT_Y;
            break;
        case 0x05:
            am = (opc.group >= 0x02 && (opc.num == 0x04 || opc.num == 0x05)) ? &AM_ZEROPAGE_Y : &AM_ZEROPAGE_X;
            break;
        case 0x06:
            am = (opc.group % 2 == 0) ? &AM_IMPLIED : &AM_ABSOLUTE_Y;
            break;
        case 0x07:
            am = (opc.group >= 0x02 && (opc.num == 0x04 || opc.num == 0x05)) ? &AM_ABSOLUTE_Y : &AM_ABSOLUTE_X;
            break;
        default:
            am = &AM_ABSOLUTE;
            break;
    }
    return am;
}

const instruction_t *get_instruction(opcode_t opc) {
    const instruction_t *ins = NULL;
    switch (opc.group) {
        case 0x00:
            ins = INS_GROUP1[opc.num][opc.addrmode];
        case 0x01:
            if (opc.num == 0x04 && opc.addrmode == 0x02) {
                ins = &INS_NOP;
            }
            else {
                ins = INS_GROUP2[opc.num];
            }
            break;
        case 0x02:
            ins = INS_GROUP3[opc.num][opc.addrmode];
            break;
        case 0x03:
            // TODO: Illegal opcodes.
            ins = NULL;
            break;
    }
    
    return ins;
}

uint8_t *fetch(const tframe_t *frame, uint8_t *mem) {
    return mem + frame->pc;
}

operation_t decode(const uint8_t *insptr) {
    // Get the opcode and args from the instruction pointer.
    const uint8_t opc = *insptr;
    const uint8_t *args = insptr + 1;

    // Convert the raw data into an opcode_t.
    opcode_converter_t decoder;
    decoder.raw = opc;

    //printf("%d %d %d\n", decoder.opcode.group, decoder.opcode.num, decoder.opcode.addrmode);

    // Decode the opcode to determine the instruction and address mode.
    operation_t result;
    result.instruction = get_instruction(decoder.opcode);
    result.addr_mode = get_address_mode(decoder.opcode);
    result.args = args;

    // If the instruction is invalid, then print an error and terminate.
    if (result.instruction == NULL) {
        printf("Invalid instruction: %d. Program terminated.\n", decoder.raw);
        exit(1);
    }

    return result;
}

void execute(tframe_t *frame, uint8_t *mem, operation_t op) {
    // If the instruction hasn't been implemented, then print an error and terminate.
    if (op.instruction->apply == NULL) {
        printf("Instruction %s not implemented. Program terminated.\n", op.instruction->name);
        exit(1);
    }

    // Evaluate the value of the instruction's argument(s) using the correct address mode.
    uint8_t *value = (uint8_t *)op.addr_mode->evaluate(frame, mem, op.args);

    // Apply the instruction using this value.
    op.instruction->apply(frame, mem, value);

    // Advance the program counter.
    frame->pc += op.addr_mode->argc + 1;
}
