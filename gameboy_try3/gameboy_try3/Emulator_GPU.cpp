#include "Emulator.h"





void Emulator::gpu_render_scanline()
{
    u8 y = reg_FF44_lineY + reg_FF42_scroll_Y; // gpu line + scroll Y

    for (u8 pixel = 0; pixel < 160; pixel++)
    {
        u8 x = pixel + reg_FF43_scroll_X; // TODO: figure out why using an int breaks everything
        //int x2 = pixel + reg_FF43_scroll_X;

        // what tile are we in?
        u16 tile_idx = (y >> 3) * 32 + (x >> 3);

        // get the tile reference out of the tile map we are currently in
        u8 tile_ref = mem_vram[(get_bit(reg_FF40_LCD_ctrl, 3) ? 0x1C00 : 0x1800) + tile_idx];

        // get the pixel from the tile depending on which tile set we're in
        int background = gpu_tileset[get_bit(reg_FF40_LCD_ctrl, 4) ? tile_ref : (256 + (int8_t)tile_ref)][y & 0x7][x & 0x7];

        gpu_screen_data[reg_FF44_lineY * 160 + pixel] = gpu_bg_palette[background];
    }

    gpu_generate_oam_entries();

    // my attempt - NO SCROLLING, ONE TILE_SET 
    for (int i = 0; i < 40; i++)
    {
        oam_entry sprite = gpu_oam_entries[i];
     
        // does the sprite fall on this line?
        if ((sprite.y_coord > reg_FF44_lineY + 16) || (sprite.y_coord + 7 < reg_FF44_lineY + 16)) continue;

        // which of its rows falls on this line?
        u8 sprite_row = reg_FF44_lineY + 16 - sprite.y_coord; // since we know this to be 0-7

        for (u8 pixel = 0; pixel < 8; pixel++)
        {
            // for every pixel in the sprite we want ot figure out where it falls on the screen 
            int screen_pos_x = sprite.x_coord + pixel - 8;

            // sprite off screen
            if ((screen_pos_x < 0) || (screen_pos_x > 159)) continue;

            int pixel_shade = gpu_tileset[sprite.tile_ref][sprite_row][pixel]; // sprites only use this area :D

            if (pixel_shade == 0) continue; // sprite is transparent

            const int screen_offset = reg_FF44_lineY * 160 + screen_pos_x;

            gpu_screen_data[screen_offset] = gpu_obj0_palette[pixel_shade];
        }
    }
}




void Emulator::gpu_generate_background_sprites()
{
    // render BG
    const bool _tile_map = get_bit(reg_FF40_LCD_ctrl, 3);
    for (int y = 0; y < 256; y++)
    {
        for (int x = 0; x < 256; x++)
        {
            // what tile are we in?
            int tilex = x >> 3;
            int tiley = y >> 3;
            int tile_idx = tiley * 32 + tilex;

            // get the tile reference out of the tile map we are currently in
            u8 tile_ref = mem_vram[(_tile_map ? 0x1C00 : 0x1800) + tile_idx];

            // get the pixel from the tile depending on which tile set we're in 
            if (get_bit(reg_FF40_LCD_ctrl, 4) == 1)
                gpu_background[y][x] = gpu_bg_palette[gpu_tileset[tile_ref][y & 0x7][x & 0x7]];
            else
                gpu_background[y][x] = gpu_bg_palette[gpu_tileset[256 + (int8_t)tile_ref][y & 0x7][x & 0x7]];
        }
    }

    // render sprites 
    gpu_generate_oam_entries();

    for (int i = 0; i < 40; i++)
    {
        oam_entry sprite = gpu_oam_entries[i];

        for (u8 row = 0; row < 8; row++)
        {
            for (u8 pixel = 0; pixel < 8; pixel++)
            {
                // for every pixel in the sprite we want ot figure out where it falls on the screen 
                int screen_pos_x = sprite.x_coord + pixel - 8;
                int screen_pos_y = sprite.y_coord + row - 16;

                // sprite off screen
                if ((screen_pos_x < 0) || (screen_pos_x > 255) || (screen_pos_y < 0) || (screen_pos_y > 255)) continue; 

                int pixel_shade = gpu_tileset[sprite.tile_ref][row][pixel]; // sprites only use this area :D

                if (pixel_shade == 0) continue; // sprite is transparent

                //if ((gpu_background[screen_pos_y][screen_pos_x] != 0) && sprite.priority()) continue;

                gpu_background[screen_pos_y][screen_pos_x] = gpu_obj0_palette[pixel_shade];
            }
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

            // get the pixel 
            gpu_tileset_view[y * 128 + x] = gpu_bg_palette[gpu_tileset[tile_idx][y & 0x7][x & 0x7]];
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
void Emulator::gpu_generate_oam_entries() // this is only to preserve my sanity while developing the scan render 
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
void Emulator::gpu_generate_background() // which of 2 bg maps are we using?
{
    const bool _tile_map = get_bit(reg_FF40_LCD_ctrl, 3);
    for (int y = 0; y < 256; y++)
    {
        for (int x = 0; x < 256; x++)
        {
            // what tile are we in?
            int tilex = x >> 3;
            int tiley = y >> 3;
            int tile_idx = tiley * 32 + tilex;

            // get the tile reference out of the tile map we are currently in
            u8 tile_ref = mem_vram[(_tile_map ? 0x1C00 : 0x1800) + tile_idx];

            // get the pixel from the tile depending on which tile set we're in 
            if (get_bit(reg_FF40_LCD_ctrl, 4) == 1)
                gpu_background[y][x] = gpu_bg_palette[gpu_tileset[tile_ref][y & 0x7][x & 0x7]];
            else
                gpu_background[y][x] = gpu_bg_palette[gpu_tileset[256 + (int8_t)tile_ref][y & 0x7][x & 0x7]];
        }
    }
}




void Emulator::update_gpu(int _cyc) // returns true if it is time to vblank
{
    // this is basically a state machine that changes state after certain # of cycles passes
    // in the end we only care about writing the entire scanline during HBLANK
    system_gpu_counter += _cyc; // run GPU for this many cycles
    switch (gpu_state)
    {
    case GPU_STATE::SCANLINE_OAM: // OAM read mode, scanline active
        if (system_gpu_counter >= 80) // Enter scanline mode 3
        {
            system_gpu_counter %= 80;
            gpu_state = GPU_STATE::SCANLINE_VRAM;
        }
        break;

    case GPU_STATE::SCANLINE_VRAM: // VRAM read mode, scanline active. Treat end of mode 3 as end of scanline
        if (system_gpu_counter >= 172) // Enter hblank
        {
            system_gpu_counter %= 172;
            gpu_state = GPU_STATE::HBLANK;
            gpu_render_scanline(); // Write a scanline to the framebuffer
        }
        break;

    case GPU_STATE::HBLANK: // Hblank. After the last hblank, push the screen data to canvas
        if (system_gpu_counter >= 204)
        {
            system_gpu_counter %= 204;
            reg_FF44_lineY++;

            if (reg_FF44_lineY >= 143) // Enter vblank
            {
                gpu_state = GPU_STATE::VBLANK;
                reg_FF0F_interrupt_flag = set_bit(reg_FF0F_interrupt_flag, 0);
                system_step_output == EMULATOR_OUTPUT::VBLANK;
            }
            else gpu_state = GPU_STATE::SCANLINE_OAM; // continue in OAM mode for next scanline
        }
        break;

    case GPU_STATE::VBLANK:
        if (system_gpu_counter >= 456)
        {
            system_gpu_counter %= 456;
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


bool    oam_entry::priority() { return get_bit(data, 7); }
bool    oam_entry::y_flip() { return get_bit(data, 6); }
bool    oam_entry::x_flip() { return get_bit(data, 5); }
bool    oam_entry::palette() { return get_bit(data, 4); }

