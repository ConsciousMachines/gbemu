#include "Emulator.h"
#include "disassembler.h"


void Emulator::gpu_render_scanline()
{
    u8 y = reg_FF44_lineY + reg_FF42_scroll_Y; // gpu line + scroll Y
    
    for (int pixel = 0; pixel < 160; pixel++)
    {
        int x = pixel + reg_FF43_scroll_X;

        // what tile are we in?
        int tilex = x >> 3;
        int tiley = y >> 3;
        int tile_idx = tiley * 32 + tilex;

        // get the tile reference out of the tile map we are currently in
        u8 tile_ref;
        if (get_bit(reg_FF40_LCD_ctrl, 3) == 0)
            tile_ref = mem_vram[0x1800 + tile_idx];
        else
            tile_ref = mem_vram[0x1C00 + tile_idx];

        // get the pixel from the tile depending on which tile set we're in
        int background;
        if (get_bit(reg_FF40_LCD_ctrl, 4) == 0)
            background = gpu_tileset[128 + tile_ref][y & 0x7][x & 0x7]; // TODO: this is wrong?
        else
            background = gpu_tileset[tile_ref][y & 0x7][x & 0x7];

        //printf("%u\n", background);
        gpu_screen_data[reg_FF44_lineY * 160 + pixel] = system_bg_palette[background];
        x++;
    }

    // GOOD TILL HERE 

    // simplified
    for (int i = 0; i < 40; i++)
    {
        if ((gpu_oam_entries[i].y_coord <= reg_FF44_lineY) && ((gpu_oam_entries[i].y_coord + 8) > reg_FF44_lineY)) // check that it falls on this line
        {
            // Where to render on the canvas
            int screen_offset = (reg_FF44_lineY * 160 + gpu_oam_entries[i].x_coord);

            // Data for this line of the sprite
            int* tilerow = gpu_tileset[gpu_oam_entries[i].tile_ref][7 - (reg_FF44_lineY - gpu_oam_entries[i].y_coord)];

            for (int x = 0; x < 8; x++)
            {
                // If this pixel is still on-screen, AND
                // if it's not colour 0 (transparent), AND
                // if this sprite has priority OR shows under the bg
                // then render the pixel
                bool on_screen_x = ((gpu_oam_entries[i].x_coord + x) >= 0) && ((gpu_oam_entries[i].x_coord + x) < 160);
                bool not_color_0 = tilerow[x] > 0;
                bool uhh = gpu_oam_entries[i].priority() || !gpu_screen_data[gpu_oam_entries[i].x_coord + x];
                if (on_screen_x && not_color_0)
                {
                    gpu_screen_data[screen_offset] = tilerow[x]; // TODO: palette
                    screen_offset++;
                }
            }
        }
    }
    /*
    // full version
    for (int i = 0; i < 1; i++)
    {
        if ((oam_entries[i].y_coord <= _line) && ((oam_entries[i].y_coord + 8) > _line)) // check that it falls on this line
        {
            // Where to render on the canvas
            int screen_offset = (_line * 160 + oam_entries[i].x_coord);

            // Data for this line of the sprite
            int* tilerow; // If the sprite is Y-flipped, use the opposite side of the tile
            if (oam_entries[i].y_flip()) tilerow = gpu_tileset[oam_entries[i].tile_num][7 - (_line - oam_entries[i].y_coord)];
            else tilerow = gpu_tileset[oam_entries[i].tile_num][_line - oam_entries[i].y_coord];

            int colour;

            for (int x = 0; x < 8; x++)
            {
                // If this pixel is still on-screen, AND
                // if it's not colour 0 (transparent), AND
                // if this sprite has priority OR shows under the bg
                // then render the pixel
                bool on_screen_x = ((oam_entries[i].x_coord + x) >= 0) && ((oam_entries[i].x_coord + x) < 160);
                bool not_color_0 = tilerow[x] > 0;
                bool uhh = oam_entries[i].priority() || !screen_data[oam_entries[i].x_coord + x];
                if (on_screen_x && not_color_0)
                {
                    // If the sprite is X-flipped, write pixels in reverse order
                    if (oam_entries[i].x_flip()) colour = tilerow[7 - x];
                    else colour = tilerow[x]; // TODO: palette

                    screen_data[screen_offset] = colour;
                    screen_offset++;
                }
            }
        }
    }
    */

    /* messS
    // location in VRAM where current tile map begins
    int offset = gpu_which_tile_map ? 0x1C00 : 0x1800;

    // now we're dealing with the tile map, which is 32 * 32 tiles
    // each row (of tiles) has 32 tiles, in 32 bytes
    // each column (of tiles) is tiles that are 32 bytes apart

    // Which line of tiles to use in the map
    offset += ((_line + _scy) & 0xFF) >> 3; // right-shift by 3 since they talk of individual lines, but there's 8 in a tile
    // Which tile to start with in the map line
    int offset_x = (_scx >> 3); // once again, this is basically % 8 to get the tile number, along x-axis


    // Which line of pixels to use in the tiles
    u8 line_y = (_line + _scy) & 7; // which line within the tile do we want?
    // Where in the tile line to start
    u8 line_x = _scx & 7; // since tile has 8 lines


    // Read tile index from the gpu_background map
    u8 tile_id = _vram[offset + offset_x];

    // If the tile data set in use is #1, the
    // indices are signed; calculate a real tile offset
    //if ((gpu_which_tile_map == 1) && (tile_id < 128)) tile_id += 256; // TODO: this is wrong for u8
    if ((gpu_which_tile_map == 1) && (tile_id > 127)) tile_id -= 128; // TODO: i think this is correct? -128->0, -1->127

    for (int i = 0; i < 160; i++)
    {
        // Re-map the tile pixel through the palette
        int pre_color = 0;// gpu_tile_sets[tile_id * 64 + line_y * 8 + line_x]; // TODO: replace this with the tile func

        int screen_offset = _line * 160 * 3 + i * 3;

        //switch ((_bg_palette >> (2 * pre_color)) & 0x3)


        // When this tile ends, read another
        line_x++; // the way we're emulating this, scrollX won't change in the middle of the line
        if (line_x == 8) // since we are doing everything in terms of tiles, we need to update which tile we're looking at.
        {
            line_x = 0;
            offset_x = (offset_x + 1) & 31;
            tile_id = _vram[offset + offset_x];
            //if ((gpu_which_tile_map == 1) && (tile_id < 128)) tile_id += 256;
            if ((gpu_which_tile_map == 1) && (tile_id > 127)) tile_id -= 128; // TODO: i think this is correct? -128->0, -1->127
            // TODO: this particular method is implemented using tile x,y as the basis. i can used pixels instead?
        }
    }
    */
}


void Emulator::update_gpu(int _cyc) // returns true if it is time to vblank
{
    // this is basically a state machine that changes state after certain # of cycles passes
    // in the end we only care about writing the entire scanline during HBLANK
    gpu_mini_timer += _cyc; // run GPU for this many cycles
    switch (gpu_state)
    {
    case GPU_STATE::SCANLINE_OAM: // OAM read mode, scanline active
        if (gpu_mini_timer >= 80) // Enter scanline mode 3
        {
            gpu_mini_timer = 0;
            gpu_state = GPU_STATE::SCANLINE_VRAM;
        }
        break;

    case GPU_STATE::SCANLINE_VRAM: // VRAM read mode, scanline active. Treat end of mode 3 as end of scanline
        if (gpu_mini_timer >= 172) // Enter hblank
        {
            gpu_mini_timer = 0;
            gpu_state = GPU_STATE::HBLANK;
            reg_FF0F_interrupt_flag = set_bit(reg_FF0F_interrupt_flag, 0);
            gpu_render_scanline(); // Write a scanline to the framebuffer
        }
        break;

    case GPU_STATE::HBLANK: // Hblank. After the last hblank, push the screen data to canvas
        if (gpu_mini_timer >= 204)
        {
            gpu_mini_timer = 0;
            reg_FF44_lineY++;

            if (reg_FF44_lineY == 143) // Enter vblank
            {
                gpu_state = GPU_STATE::VBLANK;
                //GPU._canvas.putImageData(GPU._scrn, 0, 0);
            }
            else gpu_state = GPU_STATE::SCANLINE_OAM; // continue in OAM mode for next scanline
        }
        break;

    case GPU_STATE::VBLANK:
        if (gpu_mini_timer >= 456)
        {
            gpu_mini_timer = 0;
            reg_FF44_lineY++; // scan line reg

            if (reg_FF44_lineY > 153) // Restart scanning modes
            {
                reg_FF44_lineY = 0; // scan line reg
                gpu_state = GPU_STATE::SCANLINE_OAM;
            }
        }
        break;
    }
}


void Emulator::run_step()
{
    int _before_OP = system_cycles; // cycles before op
    u8 opcode = execute_opcode();
    int delta = system_cycles - _before_OP; // cycles taken during op
    

    update_timers(delta);
    update_gpu(delta);
    update_interrupts();
}

void Emulator::run_frame()
{
    const int target_cycles = 70221;
    system_cycles %= 70221;
    while (system_cycles < target_cycles)
    {
        run_step();
    }
}

void Emulator::run_log()
{
    u8 bytes[3]{ rb(pc), rb(pc + 1),rb(pc + 2) };
    dis_line l = disassemble(bytes, 0);
    //  c h n z \tA\tB\tC\tD\tE\tH\tL\tSP\tPC\n
    fprintf(fp, " %c %c %c %c %02x %02x %02x %02x %02x %02x %02x %04x %04x %02x %s\n",
        flags.cc.c == 1 ? 'x' : '.', flags.cc.h == 1 ? 'x' : '.', flags.cc.n == 1 ? 'x' : '.', flags.cc.z == 1 ? 'x' : '.',
        regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], sp, pc,
        bytes[0], l._text);
    run_step();
}

void Emulator::update_timers(int _cyc)
{
    if (reg_FF07_tac & 0b100) // is timer enabled?
    {
        m_TimerCounter -= _cyc;

        // enough cpu clock cycles have happened to update the timer
        if (m_TimerCounter <= 0)
        {
            // reset m_TimerTracer to the correct value
            SetClockFreq();

            // timer about to overflow
            if (reg_FF05_tima >= 255)
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
    m_DividerVariable += _cyc;
    if (m_DividerVariable >= 256)
    {
        m_DividerVariable = 0;
        reg_FF04_div++;
    }
}

void Emulator::SetClockFreq()
{
    switch (reg_FF07_tac & 0b11) // is timer enabled?
    {
    case 0: m_TimerCounter = 1024; break; // freq 4096
    case 1: m_TimerCounter = 16; break;// freq 262144
    case 2: m_TimerCounter = 64; break;// freq 65536
    case 3: m_TimerCounter = 256; break;// freq 16382
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
                    switch (i)
                    {
                    case 0: pc = 0x40; break; // vblank
                    case 1: pc = 0x48; break; // lcd state
                    case 2: pc = 0x50; break; // timer 
                    case 3: pc = 0x58; break; // sound
                    case 4: pc = 0x60; break; // joypad
                    }
                }
            }
        }
    }
}



u8 Emulator::rb(uint16_t addr)
{
    for (int i = 0; i < debug_mem_breakpoints_r.size(); i++) // memory breakpoints
        if (addr == debug_mem_breakpoints_r[i]) debug_stop_running = true;

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
                    if (get_bit(reg_FF00_joypad, 5) == 0) { return system_input_keys >> 4; } // action buttons
                    else if (get_bit(reg_FF00_joypad, 4) == 0) { return system_input_keys & 0x0F; } // direction buttons
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
        if (addr == debug_mem_breakpoints_w[i]) debug_stop_running = true;

    switch (addr & 0xF000)
    {
    case 0x0000:
    case 0x1000:
        // writing to memory address 0x0 to 0x1FFF this disables writing to the ram bank. 0 disables, 0xA enables
        if (m_UsingMBC3)
        {
            if ((data & 0xF) == 0xA)
            {
                m_EnableRamBank = true;
                m_enableRTC = true;
            }

            else if (data == 0x0)
            {
                m_EnableRamBank = false;
                m_enableRTC = false;
            }
        }
        if (m_UsingMBC1)
        {
            if ((data & 0xF) == 0xA)
                m_EnableRamBank = true;
            else if (data == 0x0)
                m_EnableRamBank = false;
        }
        else if (m_UsingMBC2)
        {
            //bit 0 of upper byte must be 0
            if (0 == get_bit(addr, 8))
            {
                if ((data & 0xF) == 0xA)
                    m_EnableRamBank = true;
                else if (data == 0x0)
                    m_EnableRamBank = false;
            }
        }
        return;
    case 0x2000:
    case 0x3000:
        // if writing to a memory address between 2000 and 3FFF then we need to change rom bank
        if (m_UsingMBC3)
        {
            if (data == 0x00) data++;
            data &= 0x7f; // use 7 bits 
            system_rom_bank_number = data;
        }
        if (m_UsingMBC1) // basically writes to the low 5 bits of the rom bank reg
        {
            if (data == 0x00)
                data++;

            data &= 31;

            // Turn off the lower 5-bits.
            system_rom_bank_number &= 224;

            // Combine the written data with the register.
            system_rom_bank_number |= data;
        }
        else if (m_UsingMBC2)
        {
            data &= 0xF;
            system_rom_bank_number = data;
        }
        return;
    case 0x4000:
    case 0x5000:
        // writing to address 0x4000 to 0x5FFF switches ram banks (if enabled of course)
        if (m_UsingMBC3)
        {
            if ((data >= 0x0) && (data <= 0x3))
            {
                system_eram_bank_number = data;
            }
            else if ((data >= 0x8) && (data <= 0xC))
            {
                m_ram_is_now_RTC = true;
            }
        }
        if (m_UsingMBC1)
        {
            // are we using memory model 16/8
            if (m_UsingMemoryModel16_8) // basically writes to the top 3 bits of the rom bank reg
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
        if (m_UsingMBC3)
        {
            if (data == 0x0)
            {
                // RTC latch procedure: check if the next instruction writes 0x1 here -
                // im not sure thats even how real games do it. maybe check silver code?
            }
        }
        if (m_UsingMBC1)
        {
            // we're only interested in the first bit
            data &= 1;
            if (data == 1)
            {
                system_eram_bank_number = 0;
                m_UsingMemoryModel16_8 = false;
            }
            else m_UsingMemoryModel16_8 = true;
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
                    reg_FF07_tac = data;
                    u8 newfreq = data & 0x3;
                    if (currentfreq != newfreq)
                    {
                        SetClockFreq();
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
                        case 0: system_bg_palette[i] = WHITE; break;
                        case 1: system_bg_palette[i] = LIGHT; break;
                        case 2: system_bg_palette[i] = DARK;  break;
                        case 3: system_bg_palette[i] = BLACK; break;
                        }
                    }
                    return;
                case 0xFF48: // obj0 palette
                    reg_FF48_obj_palette_0 = data;
                    for (int i = 0; i < 4; i++)
                    {
                        switch ((data >> (i * 2)) & 3)
                        {
                        case 0: system_obj0_palette[i] = WHITE; break;
                        case 1: system_obj0_palette[i] = LIGHT; break;
                        case 2: system_obj0_palette[i] = DARK;  break;
                        case 3: system_obj0_palette[i] = BLACK; break;
                        }
                    }
                    return;
                case 0xFF49: // obj1 palette
                    reg_FF49_obj_palette_1 = data;
                    for (int i = 0; i < 4; i++)
                    {
                        switch ((data >> (i * 2)) & 3)
                        {
                        case 0: system_obj1_palette[i] = WHITE; break;
                        case 1: system_obj1_palette[i] = LIGHT; break;
                        case 2: system_obj1_palette[i] = DARK;  break;
                        case 3: system_obj1_palette[i] = BLACK; break;
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











void Emulator::gpu_generate_tileset_view()
{
    for (int y = 0; y < 192; y++)
    {
        for (int x = 0; x < 128; x++)
        {
            // what tile are we in?
            int tilex = x >> 3;
            int tiley = y >> 3;
            int tile_idx = tiley * 16 + tilex;

            // get the pixel from the tile depending on which tile set we're in 
            gpu_tileset_view[y * 128 + x] = system_bg_palette[gpu_tileset[tile_idx][y & 0b111][x & 0b111]];
        }
    }
}
void Emulator::gpu_update_tileset_from_addr(uint16_t addr) // address is between 0x0000 - 0x17FF
{
    int idx_in_tileset = addr >> 4; // index in our de-convoluted gpu_tileset
    int row = (addr >> 1) & 0b111; // first 4 bits make 16 bytes, each row is 2 bytes
    uint16_t base_addr = addr & 0xFFF0;

    // update the one relevant row 
    u8 row1 = mem_vram[base_addr + row * 2];
    u8 row2 = mem_vram[base_addr + row * 2 + 1];
    for (int j = 0; j < 8; j++) // iterate over the 8 pixels in each row to gather the color
    {
        int jj = 7 - j;
        int lo_bit = (row1 >> jj) & 0b1;
        int hi_bit = (row2 >> jj) & 0b1;

        int color = (hi_bit << 1) | lo_bit;
        gpu_tileset[idx_in_tileset][row][j] = color;
    }
}
void Emulator::gpu_construct_oam_entries() // this is only to preserve my sanity while developing the scan render 
{
    // iterate over the 40 entries of 4 bytes each starting at FE00
    for (int i = 0; i < 40; i++)
    {
        gpu_oam_entries[i].y_coord = mem_oam[i * 4 + 0];
        gpu_oam_entries[i].x_coord = mem_oam[i * 4 + 1];
        gpu_oam_entries[i].tile_ref = mem_oam[i * 4 + 2];
        gpu_oam_entries[i].data = mem_oam[i * 4 + 3];
    }
}
void Emulator::gpu_update_oam_entries(uint16_t addr, u8 data) // assume addr is 00 - 9F
{
    int entry_idx = addr >> 2; // since stride is 4 bytes 
    switch (addr & 0b11) // which of the 4 bytes do we need to modify?
    {
    case 0:gpu_oam_entries[entry_idx].y_coord = data; break;
    case 1:gpu_oam_entries[entry_idx].x_coord = data; break;
    case 2:gpu_oam_entries[entry_idx].tile_ref = data; break;
    case 3:gpu_oam_entries[entry_idx].data = data; break;
    }
}
void Emulator::gpu_generate_background(bool first) // which of 2 bg maps are we using?
{
    for (int y = 0; y < 256; y++)
    {
        for (int x = 0; x < 256; x++)
        {
            // what tile are we in?
            int tilex = x >> 3;
            int tiley = y >> 3;
            int tile_idx = tiley * 32 + tilex;

            // get the tile reference out of the tile map we are currently in
            u8 tile_ref = mem_vram[(first ? 0x1800 : 0x1C00) + tile_idx];

            // get the pixel from the tile depending on which tile set we're in 
            if (get_bit(reg_FF40_LCD_ctrl, 4) == 1)
                gpu_background[y][x] = system_bg_palette[gpu_tileset[tile_ref][y & 0b111][x & 0b111]];
            else
                gpu_background[y][x] = system_bg_palette[gpu_tileset[256 + (int8_t)tile_ref][y & 0b111][x & 0b111]];
        }
    }
}






Emulator::Emulator(const char* romName)
{
    // init memory 
    for (int i = 0; i < NUMBER_OF_ERAM_BANKS; i++) mem_eram[i] = new u8[0x2000];
    for (int i = 0; i < NUMBER_OF_ERAM_BANKS; i++) memset(mem_eram[i], 0x69, sizeof(mem_eram[i]));
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
    FILE* in = fopen(romName, "rb");
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
    case 0: m_UsingMBC1 = false; break; // not using any memory swapping
    case 1:
    case 2:
    case 3: m_UsingMBC1 = true; break;
    case 5: m_UsingMBC2 = true; break;
    case 6: m_UsingMBC2 = true; break;
    }
    // how many ram banks do we neeed, if any?
    /*
    int numRamBanks = 0;
    switch (ReadMemory(0x149))
    {
    case 0: numRamBanks = 0; break;
    case 1: numRamBanks = 1; break;
    case 2: numRamBanks = 1; break;
    case 3: numRamBanks = 4; break;
    case 4: numRamBanks = 16; break;
    }
    CreateRamBanks(numRamBanks);
    */

    // open file for debug
    fp = fopen("C:\\Users\\pwnag\\Desktop\\my_emu.txt", "w+");
}

bool    oam_entry::priority() { return get_bit(data, 7); }
bool    oam_entry::y_flip() { return get_bit(data, 6); }
bool    oam_entry::x_flip() { return get_bit(data, 5); }
bool    oam_entry::palette() { return get_bit(data, 4); }

void Emulator::key_pressed(int key) { reg_FF0F_interrupt_flag = set_bit(reg_FF0F_interrupt_flag, 4); if (key < 8) system_input_keys = reset_bit(system_input_keys, key); }
void Emulator::key_released(int key){if (key < 8) system_input_keys = set_bit(system_input_keys, key);}