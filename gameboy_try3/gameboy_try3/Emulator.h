#pragma once 

#include <stdint.h>
#include <iostream>
#include <vector>


typedef uint16_t u16;
typedef uint8_t u8;

// helpers
//            transparency       blue           green       red 
#define WHITE ((255 << 24)  | (192 << 16)  | (240 << 8)   | 196)
#define LIGHT ((255 << 24)  | (168 << 16)  | (185 << 8)   | 90 )
#define DARK  ((255 << 24)  | (110 << 16)  | (96  << 8)   | 30 )
#define BLACK ((255 << 24)  | (0   << 16)  | (27  << 8)   | 45 )
#define NUMBER_OF_ERAM_BANKS 4
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))
template <typename t> static t get_bit  (t data, int bit) { return (data >> bit) & 1; }
template <typename t> static t set_bit  (t data, int bit) { return data | (1 << bit); }
template <typename t> static t reset_bit(t data, int bit) { return data & ~(1 << bit); }
typedef int pixels[8][8]; // 64 pixels to represent one "tile"
typedef uint32_t row[256]; // one row of 256 pixels 
enum class GPU_STATE { SCANLINE_OAM = 2, SCANLINE_VRAM = 3, HBLANK = 0, VBLANK = 1 };
class oam_entry
{
public:
    u8 y_coord, x_coord, tile_ref, data;
    bool    priority();
    bool    y_flip  ();
    bool    x_flip  ();
    bool    palette ();
};


class Emulator
{
public:
#pragma region state
    // state 
    u8 regs[7]; // a, b, c, d, e, h, l;
    uint16_t sp, pc;
    union {
        struct {
            u8 pad : 4;
            u8 c : 1;
            u8 h : 1;
            u8 n : 1;
            u8 z : 1;
        } cc;
        u8 psw;
    } flags;
    // register helpers
    u8 a() { return regs[0]; }
    u8 b() { return regs[1]; }
    u8 c() { return regs[2]; }
    u8 d() { return regs[3]; }
    u8 e() { return regs[4]; }
    u8 h() { return regs[5]; }
    u8 l() { return regs[6]; }
    void a(u8 b1) { regs[0] = b1; }
    void b(u8 b1) { regs[1] = b1; }
    void c(u8 b1) { regs[2] = b1; }
    void d(u8 b1) { regs[3] = b1; }
    void e(u8 b1) { regs[4] = b1; }
    void h(u8 b1) { regs[5] = b1; }
    void l(u8 b1) { regs[6] = b1; }
    uint16_t af() { return (((uint16_t)a()) << 8) | ((uint16_t)flags.psw); }
    uint16_t bc() { return (((uint16_t)b()) << 8) | ((uint16_t)c()); }
    uint16_t de() { return (((uint16_t)d()) << 8) | ((uint16_t)e()); }
    uint16_t hl() { return (((uint16_t)h()) << 8) | ((uint16_t)l()); }
    void af(u8 b1, u8 b2) { a(b1); flags.psw = b2 & 0xF0; }
    void bc(u8 b1, u8 b2) { b(b1); c(b2); }
    void de(u8 b1, u8 b2) { d(b1); e(b2); }
    void hl(u8 b1, u8 b2) { h(b1); l(b2); }
    void af(uint16_t w) { a(w >> 8); flags.psw = w & 0xF0; }
    void bc(uint16_t w) { b(w >> 8); c((u8)w); }
    void de(uint16_t w) { d(w >> 8); e((u8)w); }
    void hl(uint16_t w) { h(w >> 8); l((u8)w); }
#pragma endregion    
#pragma region CPU
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - C P U    Z O N E 
    void print_state() const;
    void push_byte(u8 b1);
    u8 pop_byte();
    void push_word(uint16_t w1, bool MSB_highest = true);
    uint16_t pop_word(bool MSB_highest = true);
    u8 next_byte();
    uint16_t next_word(bool LSB_first = true);
    void LD_r_Im(int reg);
    void LD_r1_r2(int reg1, int reg2);
    void LD_r_pair(int reg, int pair, bool decHL = false, bool incHL = false);
    void LD_pair_r(int pair, int reg, bool decHL = false, bool incHL = false);
    void LD_HL_Im();
    void LD_A_nn(bool flip);
    void LD_A_C(bool flip);
    void LDH_A_n(bool flip);
    void LD_pair_Im(int pair);
    void LD_SP_HL();
    void LDHL_SP_n();
    void LD_nn_SP();
    void PUSH(int pair);
    void POP(int pair);
    void ADD(int reg, int mode, int ADC = 0);
    void SUB(int reg, int mode, bool SBC = false, bool CP = false);
    void AND(int reg, int mode);
    void OR(int reg, int mode);
    void XOR(int reg, int mode);
    void INC(int reg, bool HL = false);
    void DEC(int reg, bool HL = false);
    void ADD_HL(int reg);
    void ADD_SP();
    void INC_16(int reg);
    void DEC_16(int reg);
    void DAA();
    void CPL();
    void CCF();
    void SCF();
    void HALT();
    void STOP();
    void DI();
    void EI();
    void RLCA();
    void RLA();
    void RRCA();
    void RRA();
    void JP(bool condition);
    void JP_HL();
    void JR(bool condition);
    void CALL(bool condition);
    void RST(uint16_t addr);
    void RET(bool condition);
    void RETI();
    u8 execute_opcode();
    void cb_SWAP(int reg, bool HL = false);
    void cb_RLC(int reg, bool HL = false);
    void cb_RL(int reg, bool HL = false);
    void cb_RRC(int reg, bool HL = false);
    void cb_RR(int reg, bool HL = false);
    void cb_SLA(int reg, bool HL = false);
    void cb_SRA(int reg, bool HL = false);
    void cb_SRL(int reg, bool HL = false);
    void cb_BIT(int bit, int reg, bool HL = false);
    void cb_RESET(int bit, int reg, bool HL = false);
    void cb_SET(int bit, int reg, bool HL = false);
    void execute_extended_opcode();
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - E N D    C P U    Z O N E 
#pragma endregion
    // bios 
    u8 _bios[256] = { 0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
    0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
    0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
    0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
    0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
    0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
    0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
    0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
    0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xF2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
    0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
    0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
    0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3c, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x4C,
    0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
    0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50 };


    // memory map 
    u8* mem_cart = new u8[0x200000];    // contains the contents of the ROM file 
    u8* mem_vram = new u8[0x2000];      // 8 KiB : 8000 - 9FFF
    u8* mem_eram[NUMBER_OF_ERAM_BANKS]; // 8 KiB : A000 - BFFF external ram, changes by bank # for diff MBC 
    u8* mem_wram = new u8[0x2000];      // 8 KiB : C000 - DFFF 
    u8* mem_oam  = new u8[160];         // 160 b : FE00 - FE9F 
    u8* mem_hram = new u8[128];;        // 127 b : FF00 - FFFF
    u8 system_rom_bank_number  = 1;
    u8 system_eram_bank_number = 0;
    bool m_EnableRamBank = false;
    bool m_enableRTC = false;
    bool m_ram_is_now_RTC = false;
    bool m_UsingMemoryModel16_8 = false;

    FILE* fp;
    u16 m_RetraceLY = 456;
    int m_TimerCounter = 0;
    bool m_UsingMBC1 = false;
    bool m_UsingMBC2 = false;
    bool m_UsingMBC3 = false;


    Emulator(const char*);
    void run_step();
    void run_frame();
    void run_log();
    void key_pressed(int);
    void key_released(int);
    void update_gpu(int);
    void update_interrupts();
    void update_timers(int);
    void wb(uint16_t, u8);
    u8 rb(uint16_t);


    std::vector<u16> debug_mem_breakpoints_w = std::vector<u16>();
    std::vector<u16> debug_mem_breakpoints_r = std::vector<u16>();
    bool debug_stop_running = false;
    void SetClockFreq();
    int m_DividerVariable;


    int  system_cycles              = 0;
    bool system_is_stopped          = false;
    bool system_master_interrupt_en = false;
    u8   system_input_keys          = 0xFF; // 0 = right; 1 = left; 2 = up; 3 = down; 4 = a; 5 = b; 6 = sel; 7 = start 
    u8 reg_FF00_joypad              = 0xCF;
    u8 reg_FF04_div                 = 0xAB; 
    u8 reg_FF05_tima                = 0x00;
    u8 reg_FF06_tma                 = 0x00;
    u8 reg_FF07_tac                 = 0xF8;
    u8 reg_FF0F_interrupt_flag      = 0xE1;
    u8 reg_FF40_LCD_ctrl            = 0x91;
    u8 reg_FF41_LCD_stat            = 0x85;
    u8 reg_FF42_scroll_Y            = 0x00;
    u8 reg_FF43_scroll_X            = 0x00;
    u8 reg_FF44_lineY               = 0x00;
    u8 reg_FF45_lineY_compare       = 0x00;
    u8 reg_FF46_dma                 = 0xFF;
    u8 reg_FF47_bg_palette          = 0xFC;
    u8 reg_FF48_obj_palette_0       = 0x00;
    u8 reg_FF49_obj_palette_1       = 0x00;
    u8 reg_FF4A_window_Y            = 0x00;
    u8 reg_FF4B_window_X            = 0x00;
    u8 reg_FFFF_interrupt_en        = 0x00;


    // gpu
    GPU_STATE  gpu_state;
    int        gpu_mini_timer   = 0; // timer between GPU states 
    oam_entry* gpu_oam_entries  = new oam_entry[40];
    pixels*    gpu_tileset      = new pixels[384]; // list of 1024 "tiles" where each tile is an int[8][8] 
    row*       gpu_background   = new row[256]; // each pixel is a color, 0 = White, 1 = Light, 2 = Dark, 3 = Black
    uint32_t*  gpu_tileset_view = new uint32_t[384 * 8 * 8]; // display the tile set 
    uint32_t*  gpu_screen_data  = new uint32_t[144 * 160];
    uint32_t system_bg_palette[4];
    uint32_t system_obj0_palette[4];
    uint32_t system_obj1_palette[4];
    
    void gpu_render_scanline();
    void gpu_generate_tileset_view();
    void gpu_update_tileset_from_addr(uint16_t addr);
    void gpu_construct_oam_entries();
    void gpu_update_oam_entries(uint16_t addr, u8 data);
    void gpu_generate_background(bool first);
};
