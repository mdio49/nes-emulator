#include <sys.h>
#include <mappers.h>
#include <stdio.h>
#include <stdlib.h>

static uint8_t *cpu_resolve_rule(const addrspace_t *as, addr_t vaddr, uint8_t *target, size_t offset);
static uint8_t *ppu_resolve_rule(const addrspace_t *as, addr_t vaddr, uint8_t *target, size_t offset);

static uint8_t cpu_update_rule(const addrspace_t *as, addr_t vaddr, uint8_t value, uint8_t mode);
static uint8_t ppu_update_rule(const addrspace_t *as, addr_t vaddr, uint8_t value, uint8_t mode);

apu_t *apu = NULL;
cpu_t *cpu = NULL;
ppu_t *ppu = NULL;
prog_t *curprog = NULL;
tv_sys_t tv_sys = TV_SYS_NTSC;

void sys_poweron(void) {
    /* Create CPU and PPU. */
    apu = apu_create();
    cpu = cpu_create();
    ppu = ppu_create();

    /* Setup CPU address space. */

    // Work memory.
    for (int i = 0; i < 4; i++) {
        as_add_segment(cpu->as, i * WMEM_SIZE, WMEM_SIZE, cpu->wmem, AS_READ | AS_WRITE);
    }

    // PPU memory-mapped registers.
    for (int i = 0x2000; i < 0x4000; i += 8) {
        as_add_segment(cpu->as, i, 1, &ppu->controller.value, AS_WRITE);
        as_add_segment(cpu->as, i + 1, 1, &ppu->mask.value, AS_WRITE);
        as_add_segment(cpu->as, i + 2, 1, &ppu->status.value, AS_READ);
        as_add_segment(cpu->as, i + 3, 1, &ppu->oam_addr, AS_WRITE);
        as_add_segment(cpu->as, i + 4, 1, &ppu->oam_data, AS_READ | AS_WRITE);
        as_add_segment(cpu->as, i + 5, 1, &ppu->scroll, AS_WRITE);
        as_add_segment(cpu->as, i + 6, 1, &ppu->ppu_addr, AS_WRITE);
        as_add_segment(cpu->as, i + 7, 1, &ppu->ppu_data, AS_READ | AS_WRITE);
    }

    // APU registers.
    as_add_segment(cpu->as, APU_PULSE1 + 0, 1, &apu->pulse[0].reg0, AS_WRITE);
    as_add_segment(cpu->as, APU_PULSE1 + 1, 1, &apu->pulse[0].reg1, AS_WRITE);
    as_add_segment(cpu->as, APU_PULSE1 + 2, 1, &apu->pulse[0].reg2, AS_WRITE);
    as_add_segment(cpu->as, APU_PULSE1 + 3, 1, &apu->pulse[0].reg3, AS_WRITE);

    as_add_segment(cpu->as, APU_PULSE2 + 0, 1, &apu->pulse[1].reg0, AS_WRITE);
    as_add_segment(cpu->as, APU_PULSE2 + 1, 1, &apu->pulse[1].reg1, AS_WRITE);
    as_add_segment(cpu->as, APU_PULSE2 + 2, 1, &apu->pulse[1].reg2, AS_WRITE);
    as_add_segment(cpu->as, APU_PULSE2 + 3, 1, &apu->pulse[1].reg3, AS_WRITE);

    as_add_segment(cpu->as, APU_TRIANGLE + 0, 1, &apu->triangle.reg0, AS_WRITE);
    as_add_segment(cpu->as, APU_TRIANGLE + 1, 1, &apu->triangle.reg1, AS_WRITE);
    as_add_segment(cpu->as, APU_TRIANGLE + 2, 1, &apu->triangle.reg2, AS_WRITE);
    as_add_segment(cpu->as, APU_TRIANGLE + 3, 1, &apu->triangle.reg3, AS_WRITE);

    as_add_segment(cpu->as, APU_NOISE + 0, 1, &apu->noise.reg0, AS_WRITE);
    as_add_segment(cpu->as, APU_NOISE + 1, 1, &apu->noise.reg1, AS_WRITE);
    as_add_segment(cpu->as, APU_NOISE + 2, 1, &apu->noise.reg2, AS_WRITE);
    as_add_segment(cpu->as, APU_NOISE + 3, 1, &apu->noise.reg3, AS_WRITE);

    as_add_segment(cpu->as, APU_DMC + 0, 1, &apu->dmc.reg0, AS_WRITE);
    as_add_segment(cpu->as, APU_DMC + 1, 1, &apu->dmc.reg1, AS_WRITE);
    as_add_segment(cpu->as, APU_DMC + 2, 1, &apu->dmc.reg2, AS_WRITE);
    as_add_segment(cpu->as, APU_DMC + 3, 1, &apu->dmc.reg3, AS_WRITE);

    // OAM DMA.
    as_add_segment(cpu->as, OAM_DMA, 1, &cpu->oam_dma, AS_WRITE);

    // APU status.
    as_add_segment(cpu->as, APU_STATUS, 1, &apu->status.value, AS_READ | AS_WRITE);

    // Joypad registers.
    as_add_segment(cpu->as, JOYPAD1, 1, &cpu->joypad1, AS_READ | AS_WRITE);
    as_add_segment(cpu->as, JOYPAD2, 1, &cpu->joypad2, AS_READ | AS_WRITE);

    // Memory not normally used.
    as_add_segment(cpu->as, TEST_MODE, sizeof(cpu->test_mode), cpu->test_mode, 0x00);

    /* Setup PPU address space. */

    // Palette memory (not configurable by mapper).
    for (int i = 0; i < 8; i++) {
        addr_t offset = 0x3F00 + (i * 0x20);
        as_add_segment(ppu->as, offset, 1, &ppu->bkg_color, AS_READ | AS_WRITE);
        as_add_segment(ppu->as, offset + 1, 15, ppu->bkg_palette, AS_READ | AS_WRITE);
        for (int j = 0; j < 4; j++) {
            addr_t start = offset + 0x10 + (j << 2);
            as_add_segment(ppu->as, start, 1, j > 0 ? &ppu->bkg_palette[j * 4 - 1] : &ppu->bkg_color, AS_READ | AS_WRITE);
            as_add_segment(ppu->as, start + 1, 3, &ppu->spr_palette[j * 3], AS_READ | AS_WRITE);
        }
    }

    // Nametables are mirrored (i.e. $3000-$3EFF is a mirror of $2000-$2EFF).
    as_add_mirror(ppu->as, 0x3000, 0x3EFF, 0, 0x2000);

    // PPU address space is mirrored every 0x4000 bytes.
    as_add_mirror(ppu->as, 0x4000, 0x7FFF, 0, 0x0000);
    as_add_mirror(ppu->as, 0x8000, 0xBFFF, 0, 0x0000);
    as_add_mirror(ppu->as, 0xC000, 0xFFFF, 0, 0x0000);

    /* Set address space resolve and update rules. */
    as_set_resolve_rule(cpu->as, cpu_resolve_rule);
    as_set_resolve_rule(ppu->as, ppu_resolve_rule);
    
    as_set_update_rule(cpu->as, cpu_update_rule);
    as_set_update_rule(ppu->as, ppu_update_rule);
}

void sys_poweroff(void) {
    apu_destroy(apu);
    cpu_destroy(cpu);
    ppu_destroy(ppu);
    curprog = NULL;
}

void sys_reset(void) {
    // Reset CPU.
    cpu_reset(cpu);

    // Reset APU.
    apu_reset(apu);

    // Reset PPU.
    ppu_reset(ppu);
}

void sys_insert(prog_t *prog) {
    // Set the program as the current program in the NES.
    curprog = prog;

    // Initialize the program's mapper.
    mapper_init(prog->mapper, cpu->as, ppu->as, ppu->vram);

    // Invoke the mapper to initialize the address space.
    mapper_insert(prog->mapper, prog);
}

void sys_run(handlers_t *handlers) {
    // Reset the CPU (so the program counter is set correctly).
    cpu_reset(cpu);
    
    // Run the program.
    handlers->running = true;
    while (handlers->running) {
        // If emulation is paused, then update the screen so that the program responds and spin until emulation is resumed.
        if (handlers->paused) {
            handlers->update_screen(NULL);
            continue;
        }

        // Record the old state of the NMI enable flag as enabling it while VBL flag is set should delay NMI for one instruction.
        bool nmi_delay = !ppu->status.vblank || !ppu->controller.nmi;

        int cycles;
        if (cpu->oam_upload) {
            const addr_t offset = cpu->oam_dma << 8;
            for (int i = 0; i < 256; i++) {
                ppu->oam[(ppu->oam_addr + i) & 0xFF] = as_read(cpu->as, offset + i);
            }
            cycles = 513 + (cpu->cycles % 2); // Add 1 cycle on odd CPU cycle.
            cpu->oam_upload = false;
        }
        else {
            // Fetch and decode the next instruction.
            uint8_t opc = cpu_fetch(cpu);
            operation_t ins = cpu_decode(cpu, opc);

            // Handle any events that occur before the instruction is executed.
            if (handlers->before_execute != NULL) {
                handlers->before_execute(ins);
            }

            // Execute the instruction.
            cycles = cpu_execute(cpu, ins);
            
            // Handle any events that occur after the instruction is executed.
            if (handlers->after_execute != NULL) {
                handlers->after_execute(ins);
            }
        }

        // Cycle the mapper.
        mapper_cycle(curprog->mapper, curprog, cycles);  

        // Cycle the APU.
        apu_update(apu, cpu->as, cycles);

        // Check for IRQ.
        if ((apu->irq_flag || curprog->mapper->irq) && !cpu->frame.sr.irq) {
            curprog->mapper->irq = false;
            cpu_irq(cpu);
        }
        apu->irq_flag = false;

        // Check for NMI.
        if (ppu->status.vblank && ppu->controller.nmi && !(nmi_delay && ppu->controller.nmi) && !ppu->nmi_suppress && !ppu->nmi_occurred) {
            ppu->nmi_occurred = true;
            cpu_nmi(cpu);
        }

        // Increment the CPU's cycle counter.
        cpu->cycles += cycles;

        // Cycle the PPU.
        ppu_render(ppu, cycles * 3);
        if (ppu->vbl_occurred) {
            handlers->update_screen(ppu->out);
            ppu->vbl_occurred = false;
        }

        // Check for input.
        if (cpu->jp_strobe) {
            cpu->joypad1_t = handlers->poll_input_p1();
            cpu->joypad2_t = handlers->poll_input_p2();
        }

        // Store the state of next key to be checked in the joypad I/O registers.
        cpu->joypad1 = cpu->joypad1_t & 0x01;
        cpu->joypad2 = cpu->joypad2_t & 0x01;
    }
}

static uint8_t *cpu_resolve_rule(const addrspace_t *as, addr_t vaddr, uint8_t *target, size_t offset) {
    // Let the mapper remap PRG-ROM and PRG-RAM.
    if (vaddr >= PRG_RAM_START && vaddr < PRG_ROM_START) {
        target = curprog->mapper->map_ram(curprog->mapper, curprog, vaddr, target, offset);
    }
    else if (vaddr >= PRG_ROM_START) {
        target = curprog->mapper->map_prg(curprog->mapper, curprog, vaddr, target, offset);
    }

    return target;
}

static uint8_t *ppu_resolve_rule(const addrspace_t *as, addr_t vaddr, uint8_t *target, size_t offset) {
    // Let the mapper remap CHR-ROM (i.e. pattern tables) and nametaables.
    if (vaddr < NAMETABLE0) {
        target = curprog->mapper->map_chr(curprog->mapper, curprog, vaddr, target, offset);
    }
    else if (vaddr < NAMETABLE3 + NT_SIZE) {
        target = curprog->mapper->map_nts(curprog->mapper, curprog, vaddr, target, offset);
    }

    return target;
}

static uint8_t cpu_update_rule(const addrspace_t *as, addr_t vaddr, uint8_t value, uint8_t mode) {
    bool read = mode & AS_READ;
    bool write = mode & AS_WRITE;

    // Allow the mapper to monitor writes.
    if (write) {
        mapper_monitor(curprog->mapper, curprog, cpu->as, vaddr, value, true);
    }

    if ((vaddr & 0xC000) == 0 && (vaddr & 0x2000) > 0) {
        // PPU memory-mapped registers.
        switch (vaddr & 0x2007) {
            case PPU_CTRL:
                if (write) {
                    // Resolve the bitmask.
                    union ppu_ctrl ctrl;
                    ctrl.nt_addr = value & 0x03;
                    ctrl.vram_inc = (value >> 2) & 0x01;
                    ctrl.spt_addr = (value >> 3) & 0x01;
                    ctrl.bpt_addr = (value >> 4) & 0x01;
                    ctrl.spr_size = (value >> 5) & 0x01;
                    ctrl.m_slave = (value >> 6) & 0x01;
                    ctrl.nmi = (value >> 7) & 0x01;
                    value = ctrl.value;

                    // Set the write flag to true.
                    ppu->ppucontrol_flags.write = 1;
                }
                break;
            case PPU_MASK:
                if (write) {
                    // Resolve the bitmask.
                    union ppu_mask mask;
                    mask.grayscale = value & 0x01;
                    mask.bkg_left = (value >> 1) & 0x01;
                    mask.spr_left = (value >> 2) & 0x01;
                    mask.background = (value >> 3) & 0x01;
                    mask.sprites = (value >> 4) & 0x01;
                    mask.em_red = (value >> 5) & 0x01;
                    mask.em_green = (value >> 6) & 0x01;
                    mask.em_blue = (value >> 7) & 0x01;
                    value = mask.value;
                }
                break;
            case PPU_STATUS:
                if (read) {
                    // Resolve the bitmask.
                    union ppu_status status = { .value = value };
                    value = (status.vblank << 7) | (status.hit << 6) | (status.overflow << 5);

                    // Set the read flag to true.
                    ppu->ppustatus_flags.read = 1;
                }
                break;
            case PPU_SCROLL:
                if (write) {
                    ppu->ppuscroll_flags.write = 1;
                }
                break;
            case OAM_DATA:
                if (read) {
                    ppu->oamdata_flags.read = 1;
                }
                if (write) {
                    ppu->oamdata_flags.write = 1;
                }
                break;
            case PPU_ADDR:
                if (write) {
                    ppu->ppuaddr_flags.write = 1;
                }
                break;
            case PPU_DATA:
                if (read) {
                    ppu->ppudata_flags.read = 1;
                }
                if (write) {
                    ppu->ppudata_flags.write = 1;
                }
                break;
        }
    }
    else if (vaddr >= APU_PULSE1 && vaddr <= APU_DMC + 0x03) {
        // APU memory-mapped registers (excluding status).
        pulse_t pulse;
        triangle_t triangle;
        noise_t noise;
        dmc_t dmc;
        
        if (write) {
            // Set start flag of envelopes and reload flag of sweep units, and reset sequencers (if necessary).
            switch (vaddr) {
                case APU_PULSE1 + 0x01:
                    apu->pulse[0].sweep_u.reload_flag = true;
                    break;
                case APU_PULSE1 + 0x03:
                    apu->pulse[0].envelope.start_flag = true;
                    apu->pulse[0].len_counter_reload = true;
                    apu->pulse[0].sequencer = 0;
                    break;
                case APU_PULSE2 + 0x01:
                    apu->pulse[1].sweep_u.reload_flag = true;
                    break;
                case APU_PULSE2 + 0x03:
                    apu->pulse[1].envelope.start_flag = true;
                    apu->pulse[1].len_counter_reload = true;
                    apu->pulse[1].sequencer = 0;
                    break;
                case APU_TRIANGLE + 0x03:
                    apu->triangle.lin_counter_reload = true;
                    apu->triangle.len_counter_reload = true;
                    break;
                case APU_NOISE + 0x02:
                    //apu->noise.timer_reload = true;
                    break;
                case APU_NOISE + 0x03:
                    apu->noise.len_counter_reload = true;
                    break;
                case APU_DMC + 0x01:
                    apu->dmc.output_reload = true;
                    break;
            }

            // Ensure that the correct value is put into the register after evaluating bitmasks.
            switch (vaddr) {
                case APU_PULSE1:
                case APU_PULSE2:
                    pulse.vol = value & 0x0F;
                    pulse.cons = (value >> 4) & 0x01;
                    pulse.loop = (value >> 5) & 0x01;
                    pulse.duty = (value >> 6) & 0x03;
                    value = pulse.reg0;
                    break;
                case APU_PULSE1 + 0x01:
                case APU_PULSE2 + 0x01:
                    pulse.sweep.shift = value & 0x07;
                    pulse.sweep.negate = (value >> 3) & 0x01;
                    pulse.sweep.period = (value >> 4) & 0x07;
                    pulse.sweep.enabled = (value >> 7) & 0x01;
                    value = pulse.reg1;
                    break;
                case APU_PULSE1 + 0x02:
                case APU_PULSE2 + 0x02:
                    pulse.timer_low = value;
                    value = pulse.reg2;
                    break;
                case APU_PULSE1 + 0x03:
                case APU_PULSE2 + 0x03:
                    pulse.timer_high = value & 0x07;
                    pulse.len_counter_load = value >> 3;
                    value = pulse.reg3;
                    break;
                case APU_TRIANGLE:
                    triangle.lin_counter_load = value & 0x7F;
                    triangle.loop = value >> 7;
                    value = triangle.reg0;
                    break;
                case APU_TRIANGLE + 0x02:
                    triangle.timer_low = value;
                    value = triangle.reg2;
                    break;
                case APU_TRIANGLE + 0x03:
                    triangle.timer_high = value & 0x07;
                    triangle.len_counter_load = value >> 3;
                    value = triangle.reg3;
                    break;
                case APU_NOISE:
                    noise.vol = value & 0x0F;
                    noise.cons = (value >> 4) & 0x01;
                    noise.loop = (value >> 5) & 0x01;
                    value = noise.reg0;
                    break;
                case APU_NOISE + 0x02:
                    noise.period = value & 0x0F;
                    noise.mode = value >> 7;
                    value = noise.reg2;
                    break;
                case APU_NOISE + 0x03:
                    noise.len_counter_load = value >> 3;
                    value = noise.reg3;
                    break;
                case APU_DMC:
                    dmc.rate = value & 0x0F;
                    dmc.loop = (value >> 6) & 0x01;
                    dmc.irq = (value >> 7) & 0x01;
                    value = dmc.reg0;
                    break;
                case APU_DMC + 0x01:
                    dmc.load = value & 0x7F;
                    value = dmc.reg1;
                    break;
                case APU_DMC + 0x02:
                    dmc.addr = value;
                    value = dmc.reg2;
                    break;
                case APU_DMC + 0x03:
                    dmc.length = value;
                    value = dmc.reg3;
                    break;
            }
        }
        else {
            // APU registers are not meant to be read from, so just return a value of 0.
            value = 0x00;
        }
    }
    else if (vaddr == APU_STATUS) {
        union apu_status status;
        if (write) {
            // Set the channel status flags.
            status.p1 = (value & 0x01) > 0;
            status.p2 = (value & 0x02) > 0;
            status.tri = (value & 0x04) > 0;
            status.noise = (value & 0x08) > 0;
            status.dmc = (value & 0x10) > 0;
            
            // Writing doesn't change frame interrupt flag.
            status.f_irq = apu->status.f_irq;

            // Writing clears DMC interrupt flag.
            status.d_irq = false;

            // Restart the DMC sample or silence the channel if necessary.
            if (status.dmc) {
                apu->dmc.start_flag = true;
            }

            value = status.value;
        }
        else if (read) {
            // Set the channel status flags if its respective length counter is greater than 0.
            status.p1 = apu->pulse[0].len_counter > 0;
            status.p2 = apu->pulse[1].len_counter > 0;
            status.tri = apu->triangle.len_counter > 0;
            status.noise = apu->noise.len_counter > 0;

            // Set the DMC status flag if its bytes remaining is greater than 0.
            status.dmc = apu->dmc.bytes_remaining > 0;

            // Set the interrupt flags.
            status.f_irq = apu->status.f_irq;
            status.d_irq = apu->status.d_irq;

            // Reading clears the frame interrupt flag.
            apu->status.f_irq = false;

            // Return the correct value.
            value = (status.d_irq << 7) | (status.f_irq << 6) | (status.dmc << 4) | (status.noise << 3) | (status.tri << 2) | (status.p2 << 1) | status.p1;
        }
    }
    else {
        switch (vaddr) {
            case OAM_DMA:
                cpu->oam_upload = true;
                break;
            case JOYPAD1:
                if (write) {
                    cpu->jp_strobe = (value & 0x01) > 0;
                }
                else if (read) {
                    cpu->joypad1_t = 0x80 | (cpu->joypad1_t >> 1);
                }
                break;
            case JOYPAD2:
                if (write) {
                    // This is the APU frame counter.
                    apu->frame.mode = (value & 0x80) > 0;
                    apu->frame.irq = (value & 0x40) > 0;
                    apu->frame_reset = 3 + apu->cyc_carry;
                    
                    // If the interrupt inhibit flag gets set, then clear the frame interrupt flag.
                    if (apu->frame.irq) {
                        apu->status.f_irq = false;
                    }
                }
                else if (read) {
                    // This is the input from Joypad 2.
                    cpu->joypad2_t = 0x80 | (cpu->joypad2_t >> 1);
                }
                break;
        }
    }

    // Allow the mapper to monitor reads.
    if (read) {
        mapper_monitor(curprog->mapper, curprog, cpu->as, vaddr, value, false);
    }

    //printf("cpu %c: $%.4x - %.2x\n", write ? 'w' : 'r', vaddr, value);
    return value;
}

static uint8_t ppu_update_rule(const addrspace_t *as, addr_t vaddr, uint8_t value, uint8_t mode) {
    // Allow the mapper to monitor reads and writes.
    bool write = mode & AS_WRITE;
    mapper_monitor(curprog->mapper, curprog, ppu->as, vaddr, value, write);

    //printf("ppu %c: $%.4x - %.2x\n", write ? 'w' : 'r', vaddr, value);
    return value;
}
