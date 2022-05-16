#include <emu.h>

static FILE *log_fp = NULL;

void start_log(void) {
    if (log_fp != NULL)
        return;
    
    log_fp = fopen("emu.log", "w");
}

void end_log(void) {
    if (log_fp == NULL)
        return;
    
    fclose(log_fp);
    log_fp = NULL;
}

void log_ins(operation_t ins) {
    if (log_fp == NULL)
        return;

    fprintf(log_fp, "$%.4x:", cpu->frame.pc);
    fprintf(log_fp, " %.2X", ins.opc);
    if (ins.addr_mode->argc > 0)
        fprintf(log_fp, " %.2X", ins.args[0]);
    else
        fprintf(log_fp, "   ");
    if (ins.addr_mode->argc > 1)
        fprintf(log_fp, " %.2X", ins.args[1]);
    else
        fprintf(log_fp, "   ");
    fprintf(log_fp, " \t|\t");
    print_ins(log_fp, ins);
    fprintf(log_fp, "\t|\t");
    print_state(log_fp, cpu);
    fprintf(log_fp, "\t|\tPPU: %3d, %3d CPU: %llu\n", ppu->draw_x, ppu->draw_y, cpu->cycles);
}

bool is_logging(void) {
    return log_fp != NULL;
}
