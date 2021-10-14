// things i learned:
// - memory space 0 - 7fff doesnt even exist, it goes straight into the cartridge! from there mapped to
//      individual memory chips. similarly for RAM at a000 - bfff
// MEGA BUG 1: using an int instead of u8 as index into tile array gave me blank screen. always use u8!!!
// MEGA BUG 2: my JP_HL was dereferencing HL instead of just loading it into PC. Blame CPU_man
// MEDIUM BUG 3: assumed Gearboy's flag reg has same bits as CPU_man. got wrong answers a lot 
// MEDIUM BUG 4: an SBC instr, 0xDE caused blargg test 4 to hang forever. had to debug by comparing to Gearboy state
// MEDIUM BUG 5: used reset_bit but forgot to assign its result, so no interrupt happened 
// MEDIUM BUG 6: Tetris was super glitchy because I was issuing a vblank interrupt after each HBLANK
// SERIOUS BUG 7: pokemon loading screen is offset by 1 tile row but only when it's scrolling. changing numbers to u8 solved it???

// timeline
// sept - finished 8080 emu 
// oct 7 - first tile rendering!
// oct 12 - fixed stupid bug now blargg test shows! 
// oct 14 - tetris plays and pokeman loads!
// oct 15 - passed instr_timing thanks to https://www.reddit.com/r/EmuDev/comments/7emvy6/game_boy_blarggs_instruction_timing_test_fails/

// remember to include opengl32.lib in the project, even in 64bit (lol)!!!
#include "Emulator_Wrapper.h"

int main(int argc, char* argv[])
{
    Emulator_Wrapper* gb = new Emulator_Wrapper();
    return 0;
}