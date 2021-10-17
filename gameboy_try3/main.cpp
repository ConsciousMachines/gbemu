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
// RETARD BUG 8: had a == instead of a =, vblank never got assigned
// MEGA BUG 9: there was a flash in pokemon loading. turns out it calls HALT, which had some BS in it. it works fine after removing BS (now it wont pass blargg tho)

// timeline
// sept - finished 8080 emu 
// oct 7 - first tile rendering!
// oct 12 - fixed stupid bug now blargg test shows! 
// oct 14 - tetris plays and pokeman loads!
// oct 15 - passed instr_timing thanks to https://www.reddit.com/r/EmuDev/comments/7emvy6/game_boy_blarggs_instruction_timing_test_fails/
// oct 15 - passed mem_timing 1,2 - first fixed missing breaks in rb/wb, then added missing instruction cycles in cpu in order to pass
//              instr_timing using new model where increment happens AT END of rb/wb. also interrupts happen after op completes!
//              https://www.reddit.com/r/EmuDev/comments/e2grlv/struggling_to_pass_blarggs_mem_timing_rom/
//              https://www.reddit.com/r/EmuDev/comments/pnruwk/gbgbc_passing_all_cputiming_tests/


// remember to include opengl32.lib in the project, even in 64bit (lol)!!!

