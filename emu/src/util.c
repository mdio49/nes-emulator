#include <emu.h>

void dump_state(cpu_t *cpu) {
    print_state(stdout, cpu);
    printf("\n");
}

void print_state(FILE *fp, cpu_t *cpu) {
    fprintf(fp, "pc: $%.4x, a: $%.2x, x: $%.2x, y: $%.2x, sp: $%.2x, sr: ", cpu->frame.pc, cpu->frame.ac, cpu->frame.x, cpu->frame.y, cpu->frame.sp);
    fprintf(fp, cpu->frame.sr.neg ? "n" : "-");
    fprintf(fp, cpu->frame.sr.vflow ? "v" : "-");
    fprintf(fp, cpu->frame.sr.ign ? "x" : "-");
    fprintf(fp, cpu->frame.sr.brk ? "b" : "-");
    fprintf(fp, cpu->frame.sr.dec ? "d" : "-");
    fprintf(fp, cpu->frame.sr.irq ? "i" : "-");
    fprintf(fp, cpu->frame.sr.zero ? "z" : "-");
    fprintf(fp, cpu->frame.sr.carry ? "c" : "-");
    fprintf(fp, " ($%.2x)", sr_to_bits(cpu->frame.sr));
}

void print_ins(FILE *fp, operation_t ins) {
    fprintf(fp, "%s", ins.instruction->name);
    if (ins.addr_mode == &AM_IMMEDIATE) {
        fprintf(fp, " #$%.2x     ", ins.args[0]);
    }
    else if (ins.addr_mode == &AM_ACCUMULATOR) {
        fprintf(fp, " A\t\t");
    }
    else if (ins.addr_mode == &AM_ZEROPAGE || ins.addr_mode == &AM_RELATIVE) {
        fprintf(fp, " $%.2x\t\t", ins.args[0]);
    }
    else if (ins.addr_mode == &AM_ZEROPAGE_X) {
        fprintf(fp, " $%.2x,X\t", ins.args[0]);
    }
    else if (ins.addr_mode == &AM_ZEROPAGE_Y) {
        fprintf(fp, " $%.2x,Y\t", ins.args[0]);
    }
    else if (ins.addr_mode == &AM_ABSOLUTE) {
        fprintf(fp, " $%.2x%.2x\t", ins.args[1], ins.args[0]);
    }
    else if (ins.addr_mode == &AM_ABSOLUTE_X) {
        fprintf(fp, " $%.2x%.2x,X\t", ins.args[1], ins.args[0]);
    }
    else if (ins.addr_mode == &AM_ABSOLUTE_Y) {
        fprintf(fp, " $%.2x%.2x,Y\t", ins.args[1], ins.args[0]);
    }
    else if (ins.addr_mode == &AM_INDIRECT) {
        fprintf(fp, " ($%.2x%.2x)\t", ins.args[1], ins.args[0]);
    }
    else if (ins.addr_mode == &AM_INDIRECT_X) {
        fprintf(fp, " ($%.2x,X)\t", ins.args[0]);
    }
    else if (ins.addr_mode == &AM_INDIRECT_Y) {
        fprintf(fp, " ($%.2x),Y\t", ins.args[0]);
    }
    else {
        fprintf(fp, "\t\t\t");
    }
}

bool strprefix(const char *str, const char *pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}
