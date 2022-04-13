#include <sys.h>
#include <stdlib.h>

cpu_t *cpu = NULL;
ppu_t *ppu = NULL;
prog_t *curprog = NULL;

static uint8_t mapper_regs[0x1FE0];
static uint8_t prg_ram[0x2000];

void sys_poweron(void) {
    // Create CPU and PPU.
    cpu = cpu_create();
    ppu = ppu_create();

    // Setup CPU address space.
    for (int i = 0; i < 4; i++) {
        as_add_segment(cpu->as, i * WMEM_SIZE, WMEM_SIZE, cpu->wmem);
    }
    for (int i = 0x2000; i < 0x4000; i += 8) {
        as_add_segment(cpu->as, i, 8, (uint8_t*)&ppu->controller);
    }
    as_add_segment(cpu->as, 0x4000, 0x0020, cpu->apu_io_reg);
}

void sys_poweroff(void) {
    cpu_destroy(cpu);
    ppu_destroy(ppu);
    curprog = NULL;
}

void sys_reset(void) {
    cpu_reset(cpu);
}

void sys_insert(prog_t *prog) {
    // Set the program as the current program in the NES.
    curprog = prog;

    // TODO: Make the mapper responsible for this.
    //  |
    //  |
    // \|/

    // Setup CPU address space.
    as_add_segment(cpu->as, 0x4020, 0x1FE0, mapper_regs);
    as_add_segment(cpu->as, 0x6000, 0x2000, prg_ram);
    as_add_segment(cpu->as, 0x8000, 0x4000, (uint8_t*)prog->prg_rom);
    as_add_segment(cpu->as, 0xC000, 0x4000, (uint8_t*)prog->prg_rom + 0x4000);

    // Setup PPU address space.
    as_add_segment(ppu->as, 0x0000, 0x2000, (uint8_t*)prog->chr_rom);
    as_add_segment(ppu->as, 0x2000, 0x0400, ppu->vram);
    as_add_segment(ppu->as, 0x3000, 0x0400, ppu->vram);
    if (prog->header.mirroring == 1) {
        // Vertical mirroring.
        as_add_segment(ppu->as, 0x2400, 0x0400, ppu->vram + 0x0400);
        as_add_segment(ppu->as, 0x2800, 0x0400, ppu->vram);
        as_add_segment(ppu->as, 0x2C00, 0x0400, ppu->vram + 0x0400);

        as_add_segment(ppu->as, 0x3400, 0x0400, ppu->vram + 0x0400);
        as_add_segment(ppu->as, 0x3800, 0x0400, ppu->vram);
        as_add_segment(ppu->as, 0x3C00, 0x0300, ppu->vram + 0x0400);
    }
    else {
        // Horizontal mirroring.
        as_add_segment(ppu->as, 0x2400, 0x0400, ppu->vram);
        as_add_segment(ppu->as, 0x2800, 0x0400, ppu->vram + 0x0400);
        as_add_segment(ppu->as, 0x2C00, 0x0400, ppu->vram + 0x0400);

        as_add_segment(ppu->as, 0x3400, 0x0400, ppu->vram);
        as_add_segment(ppu->as, 0x3800, 0x0400, ppu->vram + 0x0400);
        as_add_segment(ppu->as, 0x3C00, 0x0300, ppu->vram + 0x0400);
    }

    // Palette memory.
    for (int i = 0; i < 8; i++) {
        addr_t offset = 0x3F00 + (i * 0x0020);
        as_add_segment(ppu->as, offset, 1, &ppu->bkg_color);
        for (int j = 0; j < N_PALETTES; j++) {
            as_add_segment(ppu->as, offset + 1 + j * 4, 3, (uint8_t*)&ppu->palettes[j]);
            as_add_segment(ppu->as, offset + (j + 1) * 4, 1, &ppu->bkg_color);
        }
    }
}

void sys_run(handlers_t *handlers) {
    // Reset the system (so the program counter is set correctly).
    sys_reset();

    // Run the program.
    handlers->running = true;
    while (handlers->running) {
        // Fetch and decode the next instruction.
        uint8_t opc = cpu_fetch(cpu);
        operation_t ins = cpu_decode(cpu, opc);

        // Handle any events that occur before the instruction is executed.
        if (handlers->before_execute != NULL) {
            handlers->before_execute(ins);
        }

        // Execute the instruction.
        cpu_execute(cpu, ins);
        
        // Handle any events that occur after the instruction is executed.
        if (handlers->after_execute != NULL) {
            handlers->after_execute(ins);
        }

        // Spin while an interrupt is taking place.
        while (handlers->interrupted);
    }
}
