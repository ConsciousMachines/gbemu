#include "Emulator.h"





void Emulator::gpu_render_scanline()
{
    if (reg_FF40_LCD_ctrl & 0x1) // bg, window enabled
    {
        // which tile data are we using?
        bool using_window = get_bit(reg_FF40_LCD_ctrl, 5) && (reg_FF4A_window_Y <= reg_FF44_lineY);
        bool unsig = get_bit(reg_FF40_LCD_ctrl, 4);

        // which background mem?
        u16 tile_map;
        u8 y;
        if (using_window)
        {
            tile_map = get_bit(reg_FF40_LCD_ctrl, 6) ? 0x1C00 : 0x1800;
            y = reg_FF44_lineY - reg_FF4A_window_Y;
        }
        else
        {
            tile_map = get_bit(reg_FF40_LCD_ctrl, 3) ? 0x1C00 : 0x1800;
            y = reg_FF42_scroll_Y + reg_FF44_lineY;
        }

        for (u8 pixel = 0; pixel < 160; pixel++)
        {
            u8 x = pixel + reg_FF43_scroll_X;// TODO: figure out why using an int breaks everything

            if (using_window && (pixel >= reg_FF4B_window_X + 7)) x = pixel - reg_FF4B_window_X + 7;
            
            // what tile are we in?
            u16 tile_idx = (y >> 3) * 32 + (x >> 3);

            // get the tile reference out of the tile map we are currently in
            u8 tile_ref = mem_vram[tile_map + tile_idx];

            u16 tile_location = (unsig ? tile_ref * 16 : 0x800 + ((int8_t)tile_ref + 128) * 16) + (y & 0x7) * 2;
            u8 data1 = mem_vram[tile_location];
            u8 data2 = mem_vram[tile_location + 1];
            int bit = 7 - (x & 0x7);
            int pre_color = (get_bit(data2, bit) << 1) | get_bit(data1, bit);
            gpu_screen_data[reg_FF44_lineY * 160 + pixel] = gpu_bg_palette[pre_color];

            // get the pixel from the tile depending on which tile set we're in
            //int background = gpu_tileset[unsig ? tile_ref : (256 + (int8_t)tile_ref)][y & 0x7][x & 0x7];
            //gpu_screen_data[reg_FF44_lineY * 160 + pixel] = gpu_bg_palette[background];
        }
    }

    
    // are sprites enabled?
    if (!get_bit(reg_FF40_LCD_ctrl, 1)) return; 

    bool use8x16 = get_bit(reg_FF40_LCD_ctrl, 2);

    for (int i = 0; i < 40; i++)
    {
        u8 y_pos = mem_oam[i * 4 + 0] - 16;
        u8 x_pos = mem_oam[i * 4 + 1] - 8;
        u8 tile_ref = mem_oam[i * 4 + 2];
        u8 attributes = mem_oam[i * 4 + 3];
        bool flip_y = get_bit(attributes, 6);
        bool flip_x = get_bit(attributes, 5);
        int sprite_size = use8x16 ? 16 : 8;

        // is it on this row?
        if ((y_pos > reg_FF44_lineY) || (y_pos + sprite_size <= reg_FF44_lineY)) continue;

        int sprite_row = reg_FF44_lineY - y_pos;
        if (flip_y) sprite_row = sprite_size - sprite_row;

        //u8 data1 = mem_vram[tile_ref * 16 + sprite_row * 2];
        //u8 data2 = mem_vram[tile_ref * 16 + sprite_row * 2 + 1];

        for (u8 pixel = 0; pixel < 8; pixel++)
        {
            int screen_pos_x = x_pos + pixel;

            if ((screen_pos_x < 0) || (screen_pos_x > 159)) continue;

            // check if pixel is hidden behind background
            if (get_bit(attributes, 7) && (gpu_screen_data[reg_FF44_lineY * 160 + screen_pos_x] != WHITE)) continue;
            uint32_t pixel_shade = gpu_tileset[tile_ref * 64 + sprite_row * 8 + (flip_x ? 7 - pixel : pixel)]; // sprites only use this area :D
            if (pixel_shade == 0) continue; // sprite is transparent
            uint32_t color = get_bit(attributes, 4) ? gpu_obj1_palette[pixel_shade] : gpu_obj0_palette[pixel_shade];
            gpu_screen_data[reg_FF44_lineY * 160 + screen_pos_x] = color;
            /*
            u8 bit = 7 - pixel;
            if (flip_x) bit = pixel;
            u8 pre_color = (get_bit(data2, bit) << 1) | get_bit(data1, bit);
            if (pre_color == 0) continue;
            uint32_t color = get_bit(attributes, 4) ? gpu_obj1_palette[pre_color] : gpu_obj0_palette[pre_color];
            gpu_screen_data[reg_FF44_lineY * 160 + screen_pos_x] = color;
            */
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
        gpu_tileset[idx_in_tileset * 64 + row * 8 + j] = color;
    }
}
void Emulator::debug_generate_tileset_view()
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
            gpu_tileset_view[y * 128 + x] = gpu_bg_palette[gpu_tileset[tile_idx * 64 + (y & 0x7) * 8 + (x & 0x7)]];
        }
    }
}
void Emulator::debug_generate_background()
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
            u8 tile_ref = mem_vram[(get_bit(reg_FF40_LCD_ctrl, 3) ? 0x1C00 : 0x1800) + tile_idx];

            // get the pixel from the tile depending on which tile set we're in 
            if (get_bit(reg_FF40_LCD_ctrl, 4) == 1)
                gpu_background[y * 256 + x] = gpu_bg_palette[gpu_tileset[tile_ref * 64 + (y & 0x7) * 8 + (x & 0x7)]];
            else
                gpu_background[y * 256 + x] = gpu_bg_palette[gpu_tileset[(256 + (int8_t)tile_ref) * 64 + (y & 0x7) * 8 + (x & 0x7)]];
        }
    }
}




void Emulator::update_gpu(int _cyc) // returns true if it is time to vblank
{
    static int gpu_counter = 0;
    // this is basically a state machine that changes state after certain # of cycles passes
    // in the end we only care about writing the entire scanline during HBLANK
    gpu_counter += _cyc; // run GPU for this many cycles
    switch (gpu_state)
    {
    case GPU_STATE::SCANLINE_OAM: // OAM read mode, scanline active
        if (gpu_counter >= 80) // Enter scanline mode 3
        {
            gpu_counter %= 80;
            gpu_state = GPU_STATE::SCANLINE_VRAM;
        }
        break;

    case GPU_STATE::SCANLINE_VRAM: // VRAM read mode, scanline active. Treat end of mode 3 as end of scanline
        if (gpu_counter >= 172) // Enter hblank
        {
            gpu_counter %= 172;
            gpu_state = GPU_STATE::HBLANK;
            gpu_render_scanline(); // Write a scanline to the framebuffer
        }
        break;

    case GPU_STATE::HBLANK: // Hblank. After the last hblank, push the screen data to canvas
        if (gpu_counter >= 204)
        {
            gpu_counter %= 204;
            reg_FF44_lineY++;

            if (reg_FF44_lineY > 143) // Enter vblank
            {
                gpu_state = GPU_STATE::VBLANK;
                system_step_output == EMULATOR_OUTPUT::VBLANK;
                reg_FF0F_interrupt_flag = set_bit(reg_FF0F_interrupt_flag, 0);
            }
            else gpu_state = GPU_STATE::SCANLINE_OAM; // continue in OAM mode for next scanline
        }
        break;

    case GPU_STATE::VBLANK:
        if (gpu_counter >= 456)
        {
            gpu_counter %= 456;
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

