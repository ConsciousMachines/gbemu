#include "Emulator.h"


void Emulator::run_step()
{
    if (system_is_halted) 
    {
        system_total_cycles += 4;
        update_timers(4);
        update_gpu(4);
        update_interrupts();
    }
    else // normal operation 
    {
        system_bonus_cycles = 0;

        // fetch - 4 cycles 
        u8 opcode = next_byte_4t();

        // execute - some # of cycles
        execute_opcode(opcode);

        // non-memory cycles, usually moving around 16bit values
        if (system_bonus_cycles > 0)
        {
            system_total_cycles += system_bonus_cycles;
            update_timers(system_bonus_cycles);
            update_gpu(system_bonus_cycles);
        }

        update_interrupts();
    }
}

void Emulator::run_to_vblank()
{
    //system_total_cycles %= 70224;
    //while ((system_total_cycles < 70224) && (system_step_output == EMULATOR_OUTPUT::NOTHING))
    while ((system_step_output != EMULATOR_OUTPUT::VBLANK))
    {
        run_step();
    }
}

void Emulator::run_to_hblank()
{
    while ((system_step_output != EMULATOR_OUTPUT::HBLANK))
    {
        run_step();
    }
}

void Emulator::update_timers(int _cyc)
{
    //static u16 reg_FF05_tima_PRECISE = 0;

    if (reg_FF07_tac & 0b100) // is timer enabled?
    {
        system_timer_counter -= _cyc;

        // enough cpu clock cycles have happened to update the timer
        while (system_timer_counter <= 0) // instr can be 24 clocks but counter can be updated each 16 clocks. hence while
        {
            // reset m_TimerTracer to the correct value
            timer_set_clock_freq();

            // timer about to overflow
            if (reg_FF05_tima == 255)//(reg_FF05_tima_PRECISE == 256)
            {
                reg_FF05_tima = reg_FF06_tma;
                //reg_FF05_tima_PRECISE = reg_FF06_tma;
                reg_FF0F_interrupt_flag |= 0b100;
            }
            else
            {
                reg_FF05_tima++;
                //reg_FF05_tima_PRECISE++;
            }
        }
    }

    // divider counter
    static u16 divider_counter = 0;
    divider_counter += _cyc;
    while (divider_counter >= 256)
    {
        divider_counter -= 256;
        reg_FF04_div++;
    }
}

void Emulator::timer_set_clock_freq()
{
    switch (reg_FF07_tac & 0b11) // these bits specify speed 
    {
    case 0: system_timer_counter += 1024; break; // freq 4096
    case 1: system_timer_counter += 16; break;// freq 262144
    case 2: system_timer_counter += 64; break;// freq 65536
    case 3: system_timer_counter += 256; break;// freq 16382
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
            system_is_halted = false; // in case it is halted

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
                    push_word_8t(pc);

                    // call the ISR
                    pc = 0x40 + i * 0x08;
                }
            }
        }
    }
}

u8 Emulator::rb(uint16_t addr, bool add_cycles)
{
    u8 ret = 0x00; 

    //for (int i = 0; i < debug_mem_breakpoints_r.size(); i++) // memory breakpoints
    //    if (addr == debug_mem_breakpoints_r[i]) system_step_output = EMULATOR_OUTPUT::BREAKPOINT;

    switch (addr & 0xF000)
    {
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000: ret = mem_cart[addr]; break;// ROM0
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000: ret = mem_cart[addr + ((system_rom_bank_number - 1) * 0x4000)]; break; // banked
    case 0x8000:
    case 0x9000: 
        /*
        if (gpu_state != GPU_STATE::SCANLINE_VRAM) // CPU can't access VRAM during gpu state 3
        {
            
        }
        */
        ret = mem_vram[addr & 0x1FFF];
        break; // Graphics: VRAM (8k)
    case 0xA000:
    case 0xB000: ret = mem_eram[system_eram_bank_number][addr & 0x1FFF]; break;// External RAM (8k)
    case 0xC000:
    case 0xD000: ret = mem_wram[addr & 0x1FFF]; break;// Working RAM (8k)
    case 0xE000: ret = mem_wram[addr & 0x1FFF]; break;// working RAM shadow
    case 0xF000:
        switch (addr & 0x0F00)
        {
        case 0x000: case 0x100: case 0x200: case 0x300:
        case 0x400: case 0x500: case 0x600: case 0x700:
        case 0x800: case 0x900: case 0xA00: case 0xB00:
        case 0xC00: case 0xD00: ret = mem_wram[addr & 0x1FFF]; break;// Working RAM shadow
        case 0xE00: // Graphics: object attribute memory
            if (addr < 0xFEA0)
            {
                /*
                if ((gpu_state == GPU_STATE::SCANLINE_OAM) || (gpu_state == GPU_STATE::SCANLINE_VRAM))
                {
                    ret = 0xFF; // CPU cannot access memory during gpu states 2,3 and returns garabge values 
                }
                else
                {
                    
                }
                */
                ret = mem_oam[addr & 0xFF];
            }
            else
            {
                ret = 0x00; // OAM is 160 bytes, remaining bytes read as 0
            }
            break;
        case 0xF00:
            if (addr <= 0xFF7F) // I/O registers
            {
                switch (addr)
                {
                case 0xFF00: // joypad reg
                    if (get_bit(reg_FF00_joypad, 5) == 0) { reg_FF0F_interrupt_flag = set_bit(reg_FF0F_interrupt_flag, 4); ret = system_input_keys >> 4; break; } // action buttons
                    else if (get_bit(reg_FF00_joypad, 4) == 0) { reg_FF0F_interrupt_flag = set_bit(reg_FF0F_interrupt_flag, 4); ret = system_input_keys & 0x0F; break; } // direction buttons
                    else { ret = 0x00; break;}
                case 0xFF04: ret = reg_FF04_div;           break;
                case 0xFF05: ret = reg_FF05_tima;          break;
                case 0xFF06: ret = reg_FF06_tma;           break;
                case 0xFF07: ret = reg_FF07_tac;           break;
                case 0xFF0F: ret = reg_FF0F_interrupt_flag;break;
                case 0xFF40: ret = reg_FF40_LCD_ctrl;      break;
                case 0xFF41: ret = reg_FF41_LCD_stat;      break;
                case 0xFF42: ret = reg_FF42_scroll_Y;      break;
                case 0xFF43: ret = reg_FF43_scroll_X;      break;
                case 0xFF44: ret = reg_FF44_lineY;         break;
                case 0xFF45: ret = reg_FF45_lineY_compare; break;
                case 0xFF46: ret = reg_FF46_dma;           break;
                case 0xFF47: ret = reg_FF47_bg_palette;    break;
                case 0xFF48: ret = reg_FF48_obj_palette_0; break;
                case 0xFF49: ret = reg_FF49_obj_palette_1; break;
                case 0xFF4A: ret = reg_FF4A_window_Y;      break;
                case 0xFF4B: ret = reg_FF4B_window_X;      break;
                default: ret = 0x00; break;// other IO not implemented yet
                }
            }
            else if (addr <= 0xFFFE) { ret = mem_hram[addr & 0x7F]; break; }// high ram 
            else { ret = reg_FFFF_interrupt_en; break; } // 0xFFFF - interrupt enable reg
        }
    }

    if (add_cycles)
    {
        system_total_cycles += 4;
        update_timers(4);
        update_gpu(4);
    }
    return ret;
}

void Emulator::wb(uint16_t addr, u8 data, bool add_cycles)
{
    //for (int i = 0; i < debug_mem_breakpoints_w.size(); i++) // memory breakpoints
    //    if (addr == debug_mem_breakpoints_w[i]) system_step_output = EMULATOR_OUTPUT::BREAKPOINT;

    switch (addr & 0xF000)
    {
    case 0x0000:
    case 0x1000:
        if (addr <= 0x1FFF)
        {
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
        }
        // no break
    case 0x2000:
    case 0x3000:
        if ((addr >= 0x2000) && (addr <= 0x3FFF))
        {
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
        }
        // no break
    case 0x4000:
    case 0x5000:
        if ((addr >= 0x4000) && (addr <= 0x5FFF))
        {
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
        }
        // no break
    case 0x6000:
    case 0x7000: 
        if ((addr >= 0x6000) && (addr <= 0x7FFF))
        {
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
        }
        break;
    case 0x8000:
    case 0x9000:
        /*
        if (gpu_state != GPU_STATE::SCANLINE_VRAM) // CPU can't access VRAM during gpu state 3
        {
            
        }
        */
        mem_vram[addr & 0x1FFF] = data;
        if (addr < 0x9800) gpu_update_tileset_from_addr(addr & 0x1FFF);
        break; // Graphics: VRAM (8k)
    case 0xA000:
    case 0xB000: mem_eram[system_eram_bank_number][addr & 0x1FFF] = data; break; // External RAM (8k)
    case 0xC000:
    case 0xD000: mem_wram[addr & 0x1FFF] = data; break; // Working RAM (8k)
    case 0xE000: mem_wram[addr & 0x1FFF] = data; break; // working RAM shadow
    case 0xF000:
        switch (addr & 0x0F00)
        {
        case 0x000: case 0x100: case 0x200: case 0x300:
        case 0x400: case 0x500: case 0x600: case 0x700:
        case 0x800: case 0x900: case 0xA00: case 0xB00:
        case 0xC00: case 0xD00: mem_wram[addr & 0x1FFF] = data; break; // Working RAM shadow
        case 0xE00: // Graphics: object attribute memory
            if (addr < 0xFEA0)
            {
                /*
                if ((gpu_state == GPU_STATE::SCANLINE_OAM) || (gpu_state == GPU_STATE::SCANLINE_VRAM))
                {
                    // CPU cannot access memory during gpu states 2,3 and returns garabge values 
                }
                else
                {
                    
                }
                */
                mem_oam[addr & 0xFF] = data;
            }
            break;
        case 0xF00:
            if (addr <= 0xFF7F) // I/O registers
            {
                switch (addr)
                {
                case 0xFF00: reg_FF00_joypad = data & 0x3F; break; // Joypad reg, only bits 4,5 write-able
                case 0xFF04: reg_FF04_div = 0;  break;
                case 0xFF05: reg_FF05_tima = data; break;
                case 0xFF06: reg_FF06_tma = data;  break;
                case 0xFF07: 
                {
                    u8 currentfreq = reg_FF07_tac & 0x3;
                    reg_FF07_tac = data & 0x7;
                    u8 newfreq = data & 0x3;
                    if (currentfreq != newfreq)
                    {
                        timer_set_clock_freq(); // is this reset necessary? instr_timing passes without it
                    }
                    break;
                }
                case 0xFF0F: reg_FF0F_interrupt_flag = data; break;
                case 0xFF40: reg_FF40_LCD_ctrl = data; break;
                case 0xFF41: reg_FF41_LCD_stat = data; break;
                case 0xFF42: reg_FF42_scroll_Y = data; break;
                case 0xFF43: reg_FF43_scroll_X = data; break;
                case 0xFF44: reg_FF44_lineY = 0; break;// Current scanline, gets reset!
                case 0xFF45: reg_FF45_lineY_compare = data; break;
                case 0xFF46: // DMA
                {
                    u16 address = data << 8; // source address is data * 100
                    for (u16 i = 0; i < 0xA0; i++) mem_oam[i] = rb(address + i, false);
                    break;
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
                    break;
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
                    break;
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
                    break;
                case 0xFF4A: reg_FF4A_window_Y = data; break;
                case 0xFF4B: reg_FF4B_window_X = data; break;
                }
            }
            else if (addr <= 0xFFFE) { mem_hram[addr & 0x7F] = data; }// high ram 
            else { reg_FFFF_interrupt_en = data; } // 0xFFFF - interrupt enable reg
            break;
        }
        break;
    }

    if (add_cycles)
    {
        system_total_cycles += 4;
        update_timers(4);
        update_gpu(4);
    }
}

Emulator::Emulator(const char* romName)
{
    // init memory 
    memset(gpu_tileset_view, 0, 384 * 8 * 8 * sizeof(uint32_t));
    memset(gpu_tileset     , 0, 384 * 8 * 8 * sizeof(uint32_t));
    memset(gpu_background  , 0, 256 * 256 * sizeof(uint32_t));
    memset(gpu_screen_data , 0, 144 * 160 * sizeof(uint32_t));
    memset(mem_cart        , 0, 0x200000);
    memset(mem_vram        , 0, 0x2000);
    memset(mem_wram        , 0, 0x2000);
    memset(mem_oam         , 0, 160);
    memset(mem_hram        , 0, 128);

    // load rom 
    FILE* in = fopen(romName, "rb"); // TODO: replace 1 array with banked array
    fread(mem_cart, 1, 0x200000, in);
    fclose(in);

    // init values in memory 
    /*
    a(0x01);
    bc(0x0013);
    de(0x00D8);
    hl(0x014D);
    flags.psw = 0xB0;
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
    }
    // create ram banks
    mem_eram = new u8*[system_total_eram_banks];
    for (int i = 0; i < system_total_eram_banks; i++) mem_eram[i] = new u8[0x2000];
    for (int i = 0; i < system_total_eram_banks; i++) memset(mem_eram[i], 0x69, sizeof(mem_eram[i]));


    // open file for debug
    //fp = fopen("C:\\Users\\pwnag\\Desktop\\my_emu.txt", "w+");
}

void Emulator::key_pressed(int key) {if (key < 8) system_input_keys = reset_bit(system_input_keys, key); }
void Emulator::key_released(int key){if (key < 8) system_input_keys = set_bit(system_input_keys, key);}

void Emulator::dump_state(char which_state)
{
    char dir[56] = "C:\\Users\\pwnag\\Desktop\\retro\\my_GBC\\my_save_state__.soy";
    dir[50] = which_state;
    //printf("%s\n", a);

    FILE* pFile;
    pFile = fopen(dir, "wb");

    // CPU state
    u8 buffer[] { flags.psw, regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], pc & 0xff, pc >> 8, sp & 0xff, sp >> 8 };
    fwrite(buffer, 1, 12, pFile);

    // memory 

    /*
    for (unsigned long long j = 0; j < 1024; ++j) {
        //Some calculations to fill a[]
        fwrite(a, 1, size * sizeof(unsigned long long), pFile);
    }
    fclose(pFile);
    */
}