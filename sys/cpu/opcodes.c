/**
 * @file opcodes.c
 * @brief Opcodes for 6502 assembler.
 * @version 1.0
 * @date 2022-03-26
 */

#include <addrmodes.h>
#include <instructions.h>

static const instruction_t *INS_GROUP1[8][8] = {
    { &INS_BRK, &INS_NOP, &INS_PHP, &INS_NOP, &INS_BPL, &INS_NOP, &INS_CLC, &INS_NOP },
    { &INS_JSR, &INS_BIT, &INS_PLP, &INS_BIT, &INS_BMI, &INS_NOP, &INS_SEC, &INS_NOP },
    { &INS_RTI, &INS_NOP, &INS_PHA, &INS_JMP, &INS_BVC, &INS_NOP, &INS_CLI, &INS_NOP },
    { &INS_RTS, &INS_NOP, &INS_PLA, &INS_JMP, &INS_BVS, &INS_NOP, &INS_SEI, &INS_NOP },
    { &INS_NOP, &INS_STY, &INS_DEY, &INS_STY, &INS_BCC, &INS_STY, &INS_TYA, &INS_SHY },
    { &INS_LDY, &INS_LDY, &INS_TAY, &INS_LDY, &INS_BCS, &INS_LDY, &INS_CLV, &INS_LDY },
    { &INS_CPY, &INS_CPY, &INS_INY, &INS_CPY, &INS_BNE, &INS_NOP, &INS_CLD, &INS_NOP },
    { &INS_CPX, &INS_CPX, &INS_INX, &INS_CPX, &INS_BEQ, &INS_NOP, &INS_SED, &INS_NOP }
};

static const instruction_t *INS_GROUP2[8] = {
	&INS_ORA,
	&INS_AND,
	&INS_EOR,
	&INS_ADC,
	&INS_STA,
	&INS_LDA,
	&INS_CMP,
	&INS_SBC
};

static const instruction_t *INS_GROUP3[8][8] = {
    { &INS_JAM, &INS_ASL, &INS_ASL, &INS_ASL, &INS_JAM, &INS_ASL, &INS_NOP, &INS_ASL },
    { &INS_JAM, &INS_ROL, &INS_ROL, &INS_ROL, &INS_JAM, &INS_ROL, &INS_NOP, &INS_ROL },
    { &INS_JAM, &INS_LSR, &INS_LSR, &INS_LSR, &INS_JAM, &INS_LSR, &INS_NOP, &INS_LSR },
    { &INS_JAM, &INS_ROR, &INS_ROR, &INS_ROR, &INS_JAM, &INS_ROR, &INS_NOP, &INS_ROR },
    { &INS_NOP, &INS_STX, &INS_TXA, &INS_STX, &INS_JAM, &INS_STX, &INS_TXS, &INS_SHX },
    { &INS_LDX, &INS_LDX, &INS_TAX, &INS_LDX, &INS_JAM, &INS_LDX, &INS_TSX, &INS_LDX },
    { &INS_NOP, &INS_DEC, &INS_DEX, &INS_DEC, &INS_JAM, &INS_DEC, &INS_NOP, &INS_DEC },
    { &INS_NOP, &INS_INC, &INS_NOP, &INS_INC, &INS_JAM, &INS_INC, &INS_NOP, &INS_INC }
};

static const instruction_t *INS_GROUP4[8][8] = {
    { &INS_SLO, &INS_SLO, &INS_ANC,  &INS_SLO, &INS_SLO, &INS_SLO, &INS_SLO, &INS_SLO },
    { &INS_RLA, &INS_RLA, &INS_ANC,  &INS_RLA, &INS_RLA, &INS_RLA, &INS_RLA, &INS_RLA },
    { &INS_SRE, &INS_SRE, &INS_ALR,  &INS_SRE, &INS_SRE, &INS_SRE, &INS_SRE, &INS_SRE },
    { &INS_RRA, &INS_RRA, &INS_ARR,  &INS_RRA, &INS_RRA, &INS_RRA, &INS_RRA, &INS_RRA },
    { &INS_SAX, &INS_SAX, &INS_ANE,  &INS_SAX, &INS_SHA, &INS_SAX, &INS_TAS, &INS_SHA },
    { &INS_LAX, &INS_LAX, &INS_LXA,  &INS_LAX, &INS_LAX, &INS_LAX, &INS_LAS, &INS_LAX },
    { &INS_DCP, &INS_DCP, &INS_SBX,  &INS_DCP, &INS_DCP, &INS_DCP, &INS_DCP, &INS_DCP },
    { &INS_ISC, &INS_ISC, &INS_USBC, &INS_ISC, &INS_ISC, &INS_ISC, &INS_ISC, &INS_ISC }
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
            break;
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
            ins = INS_GROUP4[opc.num][opc.addrmode];
            break;
    }
    
    return ins;
}
