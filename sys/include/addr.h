#ifndef ADDR_H
#define ADDR_H

#include "cpu.h"

/**
 * Memory addresses are 16-bit ranging from 0x00 to 0xFF.
 */
typedef uint16_t addr_t;

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
extern const addrmode_t AM_IMMEDIATE;

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

#endif
