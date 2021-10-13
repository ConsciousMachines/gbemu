// things i learned:
// - memory space 0 - 7fff doesnt even exist, it goes straight into the cartridge! from there mapped to
//      individual memory chips. similarly for RAM at a000 - bfff
// MEGA BUG 1: using an int instead of u8 as index into tile array gave me blank screen. always use u8!!!
// MEGA BUG 2: my JP_HL was dereferencing HL instead of just loading it into PC. Blame CPU_man
// MEDIUM BUG 3: assumed Gearboy's flag reg has same bits as CPU_man. got wrong answers a lot 
// MEDIUM BUG 4: an SBC instr, 0xDE caused blargg test 4 to hang forever. had to debug by comparing to Gearboy state
// MEDIUM BUG 5: used reset_bit but forgot to assign its result -.-

// timeline
// sept - finished 8080 emu 
// oct 7 - first tile rendering!
// oct 12 - fixed stupid bug now blargg test shows! 

// remember to include opengl32.lib in the project, even in 64bit (lol)!!!
#include "Emulator_Wrapper.h"

int main(int argc, char* argv[])
{
    Emulator_Wrapper* gb = new Emulator_Wrapper();
    return 0;
}

/*
    void gpu_update_tileset_from_idx(u8 tile_idx)
    {
        // figure out the address from the tile's reference as given by the tile map
        uint16_t address;
        int idx_in_tileset; // index in our de-convoluted gpu_tileset
        if (get_bit(reg_FF40_LCD_ctrl, 4) == 0)
        {
            idx_in_tileset = 128 + tile_idx; // TODO: this is prob wrong
            address = (int8_t)tile_idx + 0x8800; // TODO: this might be wrong
        }
        else
        {
            idx_in_tileset = tile_idx;
            address = tile_idx + 0x8000;
        }

        // iterate over rows, figure out what each pixel is
        for (int i = 0; i < 8; i++)
        {
            u8 row1 = mem_vram[address + i * 2];
            u8 row2 = mem_vram[address + i * 2 + 1];
            for (int j = 0; j < 8; j++) // iterate over the 8 pixels in each row to gather the color
            {
                int lo_bit = (row1 >> j) & 0b1;
                int hi_bit = (row2 >> j) & 0b1;

                int color = (hi_bit << 1) | lo_bit;
                if (color > 3) printf("SOY %i\n", color);
                gpu_tileset[idx_in_tileset][i][j] = color;
            }
        }
    }

*/

