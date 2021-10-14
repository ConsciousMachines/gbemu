#include "Emulator.h"


void Emulator::run_step()
{
    int t1 = system_cycles; 
    execute_opcode();
    int t2 = system_cycles - t1; 

    update_timers(t2);
    update_gpu(t2);
    update_interrupts();
}

void Emulator::run_frame()
{
    system_cycles = 0; // 4,194,304
    while ((system_cycles < 70222))// && (system_step_output == EMULATOR_OUTPUT::NOTHING))
    {
        run_step();
    }
}

void Emulator::update_timers(int _cyc)
{
    if (reg_FF07_tac & 0b100) // is timer enabled?
    {
        system_timer_counter -= _cyc;

        // enough cpu clock cycles have happened to update the timer
        if (system_timer_counter <= 0)
        {
            // reset m_TimerTracer to the correct value
            timer_set_clock_freq();

            // timer about to overflow
            if (reg_FF05_tima == 255)
            {
                reg_FF05_tima = reg_FF06_tma;
                reg_FF0F_interrupt_flag |= 0b100;
            }
            else
            {
                reg_FF05_tima++;
            }
        }
    }

    // do divider register
    system_divider_counter += _cyc;
    if (system_divider_counter > 255)
    {
        system_divider_counter = 0;
        reg_FF04_div++;
    }
}

void Emulator::timer_set_clock_freq()
{
    switch (reg_FF07_tac & 0b11) // these bits specify speed 
    {
    case 0: system_timer_counter = 1024; break; // freq 4096
    case 1: system_timer_counter = 16; break;// freq 262144
    case 2: system_timer_counter = 64; break;// freq 65536
    case 3: system_timer_counter = 256; break;// freq 16382
    }
}

void Emulator::update_interrupts()
{
    // are interrupts even enabled at all?
    if (system_master_interrupt_en)
    {
        // are there any interrupts that occured that are enabled?
        u8 todo = reg_FF0F_interrupt_flag & reg_FFFF_interrupt_en;

        if (todo)
        {
            // iterate over each bit by priority
            for (int i = 0; i < 5; i++) // only have 5 interrupts in first 5 bits
            {
                // if that bit is set 
                if ((todo >> i) & 0x1)
                {
                    // unset that bit 
                    reg_FF0F_interrupt_flag = reset_bit(reg_FF0F_interrupt_flag, i);

                    // disable further interrupts 
                    system_master_interrupt_en = false;

                    // put PC on the stack 
                    push_word(pc);

                    // call the ISR
                    pc = 0x40 + i * 0x08;
                }
            }
        }
    }
}

u8 Emulator::rb(uint16_t addr)
{
    for (int i = 0; i < debug_mem_breakpoints_r.size(); i++) // memory breakpoints
        if (addr == debug_mem_breakpoints_r[i]) system_step_output = EMULATOR_OUTPUT::BREAKPOINT;

    switch (addr & 0xF000)
    {
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000: return mem_cart[addr]; // ROM0
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000: return mem_cart[addr + ((system_rom_bank_number - 1) * 0x4000)]; // banked
    case 0x8000:
    case 0x9000: return mem_vram[addr & 0x1FFF]; // Graphics: VRAM (8k)
    case 0xA000:
    case 0xB000: return mem_eram[system_eram_bank_number][addr & 0x1FFF]; // External RAM (8k)
    case 0xC000:
    case 0xD000: return mem_wram[addr & 0x1FFF]; // Working RAM (8k)
    case 0xE000: return mem_wram[addr & 0x1FFF]; // working RAM shadow
    case 0xF000:
        switch (addr & 0x0F00)
        {
        case 0x000: case 0x100: case 0x200: case 0x300:
        case 0x400: case 0x500: case 0x600: case 0x700:
        case 0x800: case 0x900: case 0xA00: case 0xB00:
        case 0xC00: case 0xD00: return mem_wram[addr & 0x1FFF]; // Working RAM shadow
        case 0xE00: // Graphics: object attribute memory
            if (addr < 0xFEA0) return mem_oam[addr & 0xFF];
            else return 0; // OAM is 160 bytes, remaining bytes read as 0
        case 0xF00:
            if (addr <= 0xFF7F) // I/O registers
            {
                switch (addr)
                {
                case 0xFF00: // joypad reg
                    if (get_bit(reg_FF00_joypad, 5) == 0) { reg_FF0F_interrupt_flag = set_bit(reg_FF0F_interrupt_flag, 4); return system_input_keys >> 4; } // action buttons
                    else if (get_bit(reg_FF00_joypad, 4) == 0) { reg_FF0F_interrupt_flag = set_bit(reg_FF0F_interrupt_flag, 4); return system_input_keys & 0x0F; } // direction buttons
                    else { return 0x00; }
                case 0xFF04: return reg_FF04_div;
                case 0xFF05: return reg_FF05_tima;
                case 0xFF06: return reg_FF06_tma;
                case 0xFF07: return reg_FF07_tac;
                case 0xFF0F: return reg_FF0F_interrupt_flag;
                case 0xFF40: return reg_FF40_LCD_ctrl;
                case 0xFF41: return reg_FF41_LCD_stat;
                case 0xFF42: return reg_FF42_scroll_Y;
                case 0xFF43: return reg_FF43_scroll_X;
                case 0xFF44: return reg_FF44_lineY;
                case 0xFF45: return reg_FF45_lineY_compare;
                case 0xFF46: return reg_FF46_dma;
                case 0xFF47: return reg_FF47_bg_palette;
                case 0xFF48: return reg_FF48_obj_palette_0;
                case 0xFF49: return reg_FF49_obj_palette_1;
                case 0xFF4A: return reg_FF4A_window_Y;
                case 0xFF4B: return reg_FF4B_window_X;
                default: return 0; // other IO not implemented yet
                }
            }
            else if (addr <= 0xFFFE) { return mem_hram[addr & 0x7F]; }// high ram 
            else { return reg_FFFF_interrupt_en; } // 0xFFFF - interrupt enable reg
        }
    }
}

void Emulator::wb(uint16_t addr, u8 data)
{
    for (int i = 0; i < debug_mem_breakpoints_w.size(); i++) // memory breakpoints
        if (addr == debug_mem_breakpoints_w[i]) system_step_output = EMULATOR_OUTPUT::BREAKPOINT;

    switch (addr & 0xF000)
    {
    case 0x0000:
    case 0x1000:
        // writing to memory address 0x0 to 0x1FFF this disables writing to the ram bank. 0 disables, 0xA enables
        if (system_which_MBC == 3)
        {
            if ((data & 0xF) == 0xA)
            {
                system_enable_ram_banking = true;
                system_enable_RTC = true;
            }

            else if (data == 0x0)
            {
                system_enable_ram_banking = false;
                system_enable_RTC = false;
            }
        }
        if (system_which_MBC == 1)
        {
            if ((data & 0xF) == 0xA)
                system_enable_ram_banking = true;
            else if (data == 0x0)
                system_enable_ram_banking = false;
        }
        else if (system_which_MBC == 2)
        {
            //bit 0 of upper byte must be 0
            if (0 == get_bit(addr, 8))
            {
                if ((data & 0xF) == 0xA)
                    system_enable_ram_banking = true;
                else if (data == 0x0)
                    system_enable_ram_banking = false;
            }
        }
        return;
    case 0x2000:
    case 0x3000:
        // if writing to a memory address between 2000 and 3FFF then we need to change rom bank
        if (system_which_MBC == 3)
        {
            if (data == 0x00) data++;
            data &= 0x7f; // use 7 bits 
            system_rom_bank_number = data;
        }
        if (system_which_MBC == 1) // basically writes to the low 5 bits of the rom bank reg
        {
            if (data == 0x00)
                data++;

            data &= 31;

            // Turn off the lower 5-bits.
            system_rom_bank_number &= 224;

            // Combine the written data with the register.
            system_rom_bank_number |= data;
        }
        else if (system_which_MBC == 2)
        {
            data &= 0xF;
            system_rom_bank_number = data;
        }
        return;
    case 0x4000:
    case 0x5000:
        // writing to address 0x4000 to 0x5FFF switches ram banks (if enabled of course)
        if (system_which_MBC == 3)
        {
            if ((data >= 0x0) && (data <= 0x3))
            {
                system_eram_bank_number = data;
            }
            else if ((data >= 0x8) && (data <= 0xC))
            {
                system_ram_is_now_RTC = true;
            }
        }
        if (system_which_MBC == 1)
        {
            // are we using memory model 16/8
            if (system_UsingMemoryModel16_8) // basically writes to the top 3 bits of the rom bank reg
            {
                // in this mode we can only use Ram Bank 0
                system_eram_bank_number = 0;

                data &= 3;
                data <<= 5;

                if ((system_rom_bank_number & 31) == 0)
                {
                    data++;
                }

                // Turn off bits 5 and 6, and 7 if it somehow got turned on.
                system_rom_bank_number &= 31;

                // Combine the written data with the register.
                system_rom_bank_number |= data;
            }
            else system_eram_bank_number = data & 0x3;
        }
        return;
    case 0x6000:
    case 0x7000: 
        // writing to address 0x6000 to 0x7FFF switches memory model
        if (system_which_MBC == 3)
        {
            if (data == 0x0)
            {
                // RTC latch procedure: check if the next instruction writes 0x1 here -
                // im not sure thats even how real games do it. maybe check silver code?
            }
        }
        if (system_which_MBC == 1)
        {
            // we're only interested in the first bit
            data &= 1;
            if (data == 1)
            {
                system_eram_bank_number = 0;
                system_UsingMemoryModel16_8 = false;
            }
            else system_UsingMemoryModel16_8 = true;
        }
        return; 
    case 0x8000:
    case 0x9000:
        mem_vram[addr & 0x1FFF] = data;
        if (addr < 0x9800) gpu_update_tileset_from_addr(addr & 0x1FFF);
        return; // Graphics: VRAM (8k)
    case 0xA000:
    case 0xB000: mem_eram[system_eram_bank_number][addr & 0x1FFF] = data; return; // External RAM (8k)
    case 0xC000:
    case 0xD000: mem_wram[addr & 0x1FFF] = data; return; // Working RAM (8k)
    case 0xE000: mem_wram[addr & 0x1FFF] = data; return; // working RAM shadow
    case 0xF000:
        switch (addr & 0x0F00)
        {
        case 0x000: case 0x100: case 0x200: case 0x300:
        case 0x400: case 0x500: case 0x600: case 0x700:
        case 0x800: case 0x900: case 0xA00: case 0xB00:
        case 0xC00: case 0xD00: mem_wram[addr & 0x1FFF] = data; return; // Working RAM shadow
        case 0xE00: // Graphics: object attribute memory
            if (addr < 0xFEA0)
            {
                mem_oam[addr & 0xFF] = data;
                gpu_update_oam_entries(addr & 0xFF, data); // address always 0xFE..
                return;
            }
        case 0xF00:
            if (addr <= 0xFF7F) // I/O registers
            {
                switch (addr)
                {
                case 0xFF00: reg_FF00_joypad = data & 0x3F; return; // Joypad reg, only bits 4,5 write-able
                case 0xFF04: reg_FF04_div = 0;  return;
                case 0xFF05: reg_FF05_tima = data; return;
                case 0xFF06: reg_FF06_tma = data;  return;
                case 0xFF07: 
                {
                    u8 currentfreq = reg_FF07_tac & 0x3;
                    reg_FF07_tac = data & 0x7;
                    u8 newfreq = data & 0x3;
                    if (currentfreq != newfreq)
                    {
                        timer_set_clock_freq();
                    }
                    return;
                }
                case 0xFF0F: reg_FF0F_interrupt_flag = data; return;
                case 0xFF40: reg_FF40_LCD_ctrl = data; return;
                case 0xFF41: reg_FF41_LCD_stat = data; return;
                case 0xFF42: reg_FF42_scroll_Y = data; return;
                case 0xFF43: reg_FF43_scroll_X = data; return;
                case 0xFF44: reg_FF44_lineY = data; return;// Current scanline, should not happen!
                case 0xFF45: reg_FF45_lineY_compare = data; return;
                case 0xFF46: // DMA
                {
                    u16 address = data << 8; // source address is data * 100
                    for (u16 i = 0; i < 0xA0; i++)
                    {
                        mem_oam[i] = rb(address + i);
                    }
                    return;
                }
                case 0xFF47: // background palette
                    reg_FF47_bg_palette = data;
                    for (int i = 0; i < 4; i++)
                    {
                        switch ((data >> (i * 2)) & 3)
                        {
                        case 0: gpu_bg_palette[i] = WHITE; break;
                        case 1: gpu_bg_palette[i] = LIGHT; break;
                        case 2: gpu_bg_palette[i] = DARK;  break;
                        case 3: gpu_bg_palette[i] = BLACK; break;
                        }
                    }
                    return;
                case 0xFF48: // obj0 palette
                    reg_FF48_obj_palette_0 = data;
                    for (int i = 0; i < 4; i++)
                    {
                        switch ((data >> (i * 2)) & 3)
                        {
                        case 0: gpu_obj0_palette[i] = WHITE; break;
                        case 1: gpu_obj0_palette[i] = LIGHT; break;
                        case 2: gpu_obj0_palette[i] = DARK;  break;
                        case 3: gpu_obj0_palette[i] = BLACK; break;
                        }
                    }
                    return;
                case 0xFF49: // obj1 palette
                    reg_FF49_obj_palette_1 = data;
                    for (int i = 0; i < 4; i++)
                    {
                        switch ((data >> (i * 2)) & 3)
                        {
                        case 0: gpu_obj1_palette[i] = WHITE; break;
                        case 1: gpu_obj1_palette[i] = LIGHT; break;
                        case 2: gpu_obj1_palette[i] = DARK;  break;
                        case 3: gpu_obj1_palette[i] = BLACK; break;
                        }
                    }
                    return;
                case 0xFF4A: reg_FF4A_window_Y = data;
                case 0xFF4B: reg_FF4B_window_X = data;
                }
            }
            else if (addr <= 0xFFFE) { mem_hram[addr & 0x7F] = data; }// high ram 
            else { reg_FFFF_interrupt_en = data; } // 0xFFFF - interrupt enable reg
        }
    }
}

Emulator::Emulator(const char* romName)
{
    // init memory 
    memset(gpu_oam_entries, 0, 40 * sizeof(oam_entry));
    memset(gpu_tileset, 0, 384 * sizeof(pixels));
    memset(gpu_background, 0, 256 * sizeof(row));
    memset(gpu_screen_data, 0, 144 * 160 * sizeof(uint32_t));
    memset(mem_cart, 0, 0x200000);
    memset(mem_vram, 0, 0x2000);
    memset(mem_wram, 0, 0x2000);
    memset(mem_oam, 0, 160);
    memset(mem_hram, 0, 128);

    reg_FF0F_interrupt_flag = 0x11;

    // load rom 
    FILE* in = fopen(romName, "rb"); // TODO: replace 1 array with banked array
    fread(mem_cart, 1, 0x200000, in);
    fclose(in);

    // copy bios 
    //memcpy(&_game_cartridge[0], &_bios[0], 256);

    // init values in memory 
    /*
    a(0x01);
    bc(0x0013);
    de(0x00D8);
    hl(0x014D);
    flags.psw = 0xB0;
    pc = 0x100;
    sp = 0xFFFE;
    */
    a(0x11);
    bc(0x0000);
    de(0xFF56);
    hl(0x000D);
    flags.psw = 0x80;
    pc = 0x100;
    sp = 0xFFFE;

    // what kinda rom switching are we using, if any?
    switch (mem_cart[0x147])
    {
    case 0x00: system_which_MBC = 0; break; // not using any memory swapping
    case 0x01: 
    case 0x02: 
    case 0x03: system_which_MBC = 1; break;
    case 0x05: 
    case 0x06: system_which_MBC = 2; break;
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13: system_which_MBC = 3; break;

    }


    // how many rom banks do we need?
    switch (mem_cart[0x148])
    {
    case 0x00: system_total_rom_banks = 2; break;
    case 0x01: system_total_rom_banks = 4; break;
    case 0x02: system_total_rom_banks = 8; break;
    case 0x03: system_total_rom_banks = 16; break;
    case 0x04: system_total_rom_banks = 32; break;
    case 0x05: system_total_rom_banks = 64; break;
    case 0x06: system_total_rom_banks = 128; break;
    case 0x07: system_total_rom_banks = 256; break;
    case 0x08: system_total_rom_banks = 512; break;
    }


    // how many ram banks do we neeed, if any?
    switch (mem_cart[0x149])
    {
    case 0x00: system_total_eram_banks = 0; break;
    case 0x02: system_total_eram_banks = 1; break;
    case 0x03: system_total_eram_banks = 4; break;
    case 0x04: system_total_eram_banks = 16; break;
    case 0x05: system_total_eram_banks = 8; break;
    default: system_total_eram_banks = 1; break; // 2 b safe
    }
    // create ram banks
    mem_eram = new u8*[system_total_eram_banks];
    for (int i = 0; i < system_total_eram_banks; i++) mem_eram[i] = new u8[0x2000];
    for (int i = 0; i < system_total_eram_banks; i++) memset(mem_eram[i], 0x69, sizeof(mem_eram[i]));


    // open file for debug
    fp = fopen("C:\\Users\\pwnag\\Desktop\\my_emu.txt", "w+");
}

void Emulator::key_pressed(int key) {if (key < 8) system_input_keys = reset_bit(system_input_keys, key); }
void Emulator::key_released(int key){if (key < 8) system_input_keys = set_bit(system_input_keys, key);}