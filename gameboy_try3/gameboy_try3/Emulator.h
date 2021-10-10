#pragma once 

#include <stdint.h>
#include <iostream>

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
void UNIMPLEMENTED_INSTRUCTION() { printf("UNIMPLEMENTED_INSTRUCTION\n"); }
template <typename t> t get_bit  (t data, int bit) { return (data >> bit) & 1; }
template <typename t> t set_bit  (t data, int bit) { return data | (1 << bit); }
template <typename t> t reset_bit(t data, int bit) { return data & ~(1 << bit); }
typedef int pixels[8][8]; // 64 pixels to represent one "tile"
typedef uint32_t row[256]; // one row of 256 pixels 
enum class GPU_STATE { SCANLINE_OAM = 2, SCANLINE_VRAM = 3, HBLANK = 0, VBLANK = 1 };
class oam_entry
{
public:
    u8 y_coord, x_coord, tile_ref, data;
    bool    priority()   { return get_bit(data, 7); }
    bool    y_flip  ()   { return get_bit(data, 6); }
    bool    x_flip  ()   { return get_bit(data, 5); }
    bool    palette ()   { return get_bit(data, 4); }
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
    void af(u8 b1, u8 b2) { a(b1); flags.psw = b2; }
    void bc(u8 b1, u8 b2) { b(b1); c(b2); }
    void de(u8 b1, u8 b2) { d(b1); e(b2); }
    void hl(u8 b1, u8 b2) { h(b1); l(b2); }
    void af(uint16_t w) { a(((u8)((w & 0xff00) >> 8))); flags.psw = ((u8)(w & 0x00ff)); }
    void bc(uint16_t w) { b(((u8)((w & 0xff00) >> 8))); c(((u8)(w & 0x00ff))); }
    void de(uint16_t w) { d(((u8)((w & 0xff00) >> 8))); e(((u8)(w & 0x00ff))); }
    void hl(uint16_t w) { h(((u8)((w & 0xff00) >> 8))); l(((u8)(w & 0x00ff))); }
#pragma endregion    
#pragma region CPU
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - C P U    Z O N E 
    void print_state()
    {
        printf(" c h n z \tA\tB\tC\tD\tE\tH\tL\tSP\tPC\n %c %c %c %c \t%02x\t%02x\t%02x\t%02x\t%02x\t%02x\t%02x\t%04x\t%04x\t\n\n",
            flags.cc.c == 1 ? 'x' : '.', flags.cc.h == 1 ? 'x' : '.', flags.cc.n == 1 ? 'x' : '.', flags.cc.z == 1 ? 'x' : '.',
            regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], sp, pc);
    }
    void push_byte(u8 b1)
    {
        sp--;
        wb(sp, b1);
    }
    u8 pop_byte()
    {
        u8 ret = rb(sp);
        sp++;
        return ret;
    }
    void push_word(uint16_t w1, bool MSB_highest = true)
    {
        if (MSB_highest)
        {
            push_byte(w1 >> 8);
            push_byte(((u8)w1));
        }
        else
        {
            push_byte(((u8)w1));
            push_byte(w1 >> 8);
        }
    }
    uint16_t pop_word(bool MSB_highest = true)
    {
        u8 b1 = pop_byte();
        u8 b2 = pop_byte();
        if (MSB_highest) return (((uint16_t)b2) << 8) | ((uint16_t)b1);
        else return (((uint16_t)b1) << 8) | ((uint16_t)b2);
    }
    u8 next_byte()
    {
        u8 ret = rb(pc);
        pc++;
        return ret;
    }
    uint16_t next_word(bool LSB_first = true)
    {
        u8 b1 = next_byte();
        u8 b2 = next_byte();
        if (LSB_first) return (((uint16_t)b2) << 8) | ((uint16_t)b1);
        else return (((uint16_t)b1) << 8) | ((uint16_t)b2);
    }
    void LD_r_Im(int reg)
    {
        u8 immediate = next_byte();
        regs[reg] = immediate;
        system_cycles += 8;
    }
    void LD_r1_r2(int reg1, int reg2)
    {
        regs[reg1] = regs[reg2];
        system_cycles += 4;
    }
    void LD_r_pair(int reg, int pair, bool decHL = false, bool incHL = false)
    {
        u8 value = 0;
        switch (pair)
        {
        case 0: value = rb(af()); break;
        case 1: value = rb(bc()); break;
        case 2: value = rb(de()); break;
        case 3:
            value = rb(hl());
            if (decHL) hl(hl() - 1);
            if (incHL) hl(hl() + 1);
            break;
        }
        regs[reg] = value;
        system_cycles += 8;
    }
    void LD_pair_r(int pair, int reg, bool decHL = false, bool incHL = false)
    {
        switch (pair)
        {
        case 0: wb(af(), regs[reg]); break;
        case 1: wb(bc(), regs[reg]); break;
        case 2: wb(de(), regs[reg]); break;
        case 3:
            wb(hl(), regs[reg]);
            if (decHL) hl(hl() - 1);
            if (incHL) hl(hl() + 1);
            break;
        }
        system_cycles += 8;
    }
    void LD_HL_Im()
    {
        wb(hl(), next_byte());
        system_cycles += 12;
    }
    void LD_A_nn(bool flip)
    {
        system_cycles += 16;
        uint16_t addr = next_word();
        if (flip) wb(addr, regs[0]);
        else regs[0] = rb(addr);
    }
    void LD_A_C(bool flip)
    {
        if (flip) wb(0xff00 + c(), a());
        else regs[0] = rb(0xff00 + c());
        system_cycles += 8;
    }
    void LDH_A_n(bool flip)
    {
        u8 n = next_byte();
        if (flip) wb(0xff00 + n, a());
        else a(rb(0xff00 + n));
        system_cycles += 12;
    }
    void LD_pair_Im(int pair)
    {
        uint16_t word = next_word();
        switch (pair)
        {
        case 0: af(word); break;
        case 1: bc(word); break;
        case 2: de(word); break;
        case 3: hl(word); break;
        case 4: sp = word; break;
        }
        system_cycles += 12;
    }
    void LD_SP_HL()
    {
        sp = hl();
        system_cycles += 8;
    }
    void LDHL_SP_n()
    {
        int8_t n = next_byte(); // is this even being properly converted from signed to unsigned?
        hl(sp + n);
        flags.cc.z = 0;
        flags.cc.n = 0;
        // TODO: wtf with flags h/c p 77. also should address be dereferenced?
        system_cycles += 12;
    }
    void LD_nn_SP()
    {
        uint16_t addr = next_word();
        wb(addr, sp);
        wb(addr + 1, sp >> 8); // TODO: this is probably wrong
        system_cycles += 20;
    }
    void PUSH(int pair)
    {
        switch (pair)
        {
        case 0: push_word(af()); break;
        case 1: push_word(bc()); break;
        case 2: push_word(de()); break;
        case 3: push_word(hl()); break;
        }
        system_cycles += 16;
    }
    void POP(int pair)
    {
        switch (pair)
        {
        case 0: af(pop_word()); break;
        case 1: bc(pop_word()); break;
        case 2: de(pop_word()); break;
        case 3: hl(pop_word()); break;
        }
        system_cycles += 12;
    }

    void ADD(int reg, int mode, int ADC = 0) // ADD, ADC
    {
        u8 addend;
        switch (mode)
        {
        case 0: addend = regs[reg]; system_cycles += 4; break;
        case 1: addend = rb(hl()); system_cycles += 8; break;
        case 2: addend = next_byte(); system_cycles += 8; break;
        default: exit(420);
        }
        uint16_t full = ((uint16_t)regs[0]) + ((uint16_t)addend) + ADC * flags.cc.c;
        u8 half = (regs[0] & 0x0f) + (addend & 0x0f) + ADC * flags.cc.c;
        u8 answer = ((u8)full);
        flags.cc.z = answer == 0 ? 1 : 0;
        flags.cc.n = 0;
        flags.cc.c = full > 0xff ? 1 : 0;
        flags.cc.h = half > 0x0f ? 1 : 0;
        regs[0] = answer;
    }
    void SUB(int reg, int mode, int SBC = 0, bool CP = false) // SUB, SBC, CP
    {
        u8 addend;
        switch (mode)
        {
        case 0: addend = regs[reg]; system_cycles += 4; break;
        case 1: addend = rb(hl()); system_cycles += 8; break;
        case 2: addend = next_byte(); system_cycles += 8; break;
        default: exit(420);
        }
        uint16_t full = ((uint16_t)regs[0]) - ((uint16_t)addend) - SBC * flags.cc.c;
        u8 half = (regs[0] & 0x0f) - (addend & 0x0f) - SBC * flags.cc.c;
        u8 answer = ((u8)full);
        flags.cc.z = answer == 0 ? 1 : 0;
        flags.cc.n = 1;
        flags.cc.c = regs[0] < addend ? 1 : 0;
        flags.cc.h = (regs[0] & 0x0f) < (addend & 0x0f) ? 1 : 0;
        if (!CP) regs[0] = answer;
    }
    void AND(int reg, int mode)
    {
        u8 addend;
        switch (mode)
        {
        case 0: addend = regs[reg]; system_cycles += 4; break;
        case 1: addend = rb(hl()); system_cycles += 8; break;
        case 2: addend = next_byte(); system_cycles += 8; break;
        default: exit(420);
        }
        u8 answer = regs[0] & addend;
        flags.cc.z = answer == 0 ? 1 : 0;
        flags.cc.n = 0;
        flags.cc.c = 0;
        flags.cc.h = 1;
        regs[0] = answer;
    }
    void OR(int reg, int mode)
    {
        u8 addend;
        switch (mode)
        {
        case 0: addend = regs[reg]; system_cycles += 4; break;
        case 1: addend = rb(hl()); system_cycles += 8; break;
        case 2: addend = next_byte(); system_cycles += 8; break;
        default: exit(420);
        }
        u8 answer = regs[0] | addend;
        flags.cc.z = answer == 0 ? 1 : 0;
        flags.cc.n = 0;
        flags.cc.c = 0;
        flags.cc.h = 0;
        regs[0] = answer;
    }
    void XOR(int reg, int mode)
    {
        u8 addend;
        switch (mode)
        {
        case 0: addend = regs[reg]; system_cycles += 4; break;
        case 1: addend = rb(hl()); system_cycles += 8; break;
        case 2: addend = next_byte(); system_cycles += 8; break;
        default: exit(420);
        }
        u8 answer = regs[0] ^ addend;
        flags.cc.z = answer == 0 ? 1 : 0;
        flags.cc.n = 0;
        flags.cc.c = 0;
        flags.cc.h = 0;
        regs[0] = answer;
    }
    void INC(int reg, bool HL = false)
    {
        u8 value;
        if (HL) value = rb(hl());
        else value = regs[reg];
        u8 old_bit3 = value & 0x08;
        value++;
        u8 new_bit3 = value & 0x08;
        flags.cc.z = value == 0 ? 1 : 0;
        flags.cc.n = 0;
        flags.cc.h = (old_bit3 == 0x08) && (new_bit3 == 0x00);
        if (HL)
        {
            wb(hl(), value);
            system_cycles += 12;
        }
        else
        {
            regs[reg] = value;
            system_cycles += 4;
        }
    }
    void DEC(int reg, bool HL = false)
    {
        u8 value;
        if (HL) value = rb(hl());
        else value = regs[reg];
        flags.cc.h = ((value & 0x0f) == 0);
        value--;
        flags.cc.z = value == 0 ? 1 : 0;
        flags.cc.n = 1;
        if (HL)
        {
            wb(hl(), value);
            system_cycles += 4;
        }
        else
        {
            regs[reg] = value;
            system_cycles += 12;
        }
    }
    void ADD_HL(int reg)
    {
        uint16_t addend;
        switch (reg)
        {
        case 1: addend = bc(); break;
        case 2: addend = de(); break;
        case 3: addend = hl(); break;
        case 4: addend = sp;   break;
        default: exit(420);
        }
        uint16_t answer = hl() + addend;
        flags.cc.n = 0;
        flags.cc.h = 1; // TODO: set if carry from bit 11, p 90
        flags.cc.c = 1; // TODO: set if carry from bit 15
        hl(answer);
        system_cycles += 8;
    }
    void ADD_SP()
    {
        int8_t n = next_byte();
        sp += n;
        flags.cc.z = 0;
        flags.cc.n = 0;
        flags.cc.h = 1; // TODO: wtf, p 91
        flags.cc.c = 1; // TODO: wtf
        system_cycles += 16;
    }
    void INC_16(int reg)
    {
        switch (reg)
        {
        case 1: bc(bc() + 1); break;
        case 2: de(de() + 1); break;
        case 3: hl(hl() + 1); break;
        case 4: sp++;        break;
        }
        system_cycles += 8;
    }
    void DEC_16(int reg)
    {
        switch (reg)
        {
        case 1: bc(bc() - 1); break;
        case 2: de(de() - 1); break;
        case 3: hl(hl() - 1); break;
        case 4: sp--;         break;
        }
        system_cycles += 8;
    }

    void DAA()
    {
        // TODO
        flags.cc.z = regs[0] == 0;
        flags.cc.h = 0;
        system_cycles += 4;
    }
    void CPL()
    {
        a(~a());
        flags.cc.n = 1;
        flags.cc.h = 1;
        system_cycles += 4;
    }
    void CCF()
    {
        flags.cc.c = ~flags.cc.c;
        flags.cc.n = 0;
        flags.cc.h = 0;
        system_cycles += 4;
    }
    void SCF()
    {
        flags.cc.c = 1;
        flags.cc.n = 0;
        flags.cc.h = 0;
        system_cycles += 4;
    }
    void HALT()
    {
        // TODO: make this wait for an interrupt
        system_cycles += 4;
    }
    void STOP()
    {
        // TODO: make this wait for button press 
        system_cycles += 4;
        pc++;
    }
    void DI()
    {
        system_master_interrupt_en = false;
        system_cycles += 4;
    }
    void EI()
    {
        system_master_interrupt_en = true;
        system_cycles += 4;
    }
    void RLCA()
    {
        system_cycles += 8;
        // https://hax.iimarckus.org/topic/1617/
        u8 bit7 = (a() >> 7);
        a((a() << 1) | bit7);
        flags.cc.c = bit7;
        flags.cc.z = a() == 0 ? 1 : 0;
        flags.cc.n = 0;
        flags.cc.h = 0;
    }
    void RLA()
    {
        u8 bit7 = (a() >> 7);
        a((a() << 1) | flags.cc.c);
        flags.cc.c = bit7;
        flags.cc.z = a() == 0 ? 1 : 0;
        flags.cc.n = 0;
        flags.cc.h = 0;
        system_cycles += 4;
    }
    void RRCA()
    {
        u8 old_bit0 = a() & 0x1;
        a((a() >> 1));
        flags.cc.c = old_bit0;
        flags.cc.z = a() == 0 ? 1 : 0;
        flags.cc.n = 0;
        flags.cc.h = 0;
        system_cycles += 4;
    }
    void RRA()
    {
        u8 old_bit0 = a() & 0x1;
        a((a() >> 1) | (((u8)flags.cc.c) << 7));
        flags.cc.c = old_bit0;
        flags.cc.z = a() == 0 ? 1 : 0;
        flags.cc.n = 0;
        flags.cc.h = 0;
        system_cycles += 4;
    }
    void JP(bool condition)
    {
        system_cycles += 12;
        uint16_t addr = next_word();
        if (condition) pc = addr;
    }
    void JP_HL()
    {
        pc = rb(hl());
        system_cycles += 4;
    }
    void JR(bool condition)
    {
        int8_t offset = next_byte();
        system_cycles += 8;
        if (condition) pc += offset;
    }
    void CALL(bool condition)
    {
        system_cycles += 12;
        uint16_t addr = next_word();
        if (condition)
        {
            push_word(pc);
            pc = addr;
        }
    }
    void RST(uint16_t addr)
    {
        push_word(pc);
        pc = addr;
        system_cycles += 32;
    }
    void RET(bool condition)
    {
        if (condition) pc = pop_word();
        system_cycles += 8;
    }
    void RETI()
    {
        uint16_t addr = pop_word();
        pc = addr;
        system_master_interrupt_en = true;
        system_cycles += 8;
    }

    u8 execute_opcode()
    {
        u8 opcode = rb(pc);
        pc++;
        switch (opcode)
        {
            // NOP
        case 0x00: system_cycles += 4; break;
            // 1. Loads
        case 0x06: LD_r_Im(1); break;
        case 0x0e: LD_r_Im(2); break;
        case 0x16: LD_r_Im(3); break;
        case 0x1e: LD_r_Im(4); break;
        case 0x26: LD_r_Im(5); break;
        case 0x2e: LD_r_Im(6); break;
        case 0x3e: LD_r_Im(0); break;
        case 0x40: LD_r1_r2(1, 1); break;
        case 0x41: LD_r1_r2(1, 2); break;
        case 0x42: LD_r1_r2(1, 3); break;
        case 0x43: LD_r1_r2(1, 4); break;
        case 0x44: LD_r1_r2(1, 5); break;
        case 0x45: LD_r1_r2(1, 6); break;
        case 0x47: LD_r1_r2(1, 0); break;
        case 0x48: LD_r1_r2(2, 1); break;
        case 0x49: LD_r1_r2(2, 2); break;
        case 0x4a: LD_r1_r2(2, 3); break;
        case 0x4b: LD_r1_r2(2, 4); break;
        case 0x4c: LD_r1_r2(2, 5); break;
        case 0x4d: LD_r1_r2(2, 6); break;
        case 0x4f: LD_r1_r2(2, 0); break;
        case 0x50: LD_r1_r2(3, 1); break;
        case 0x51: LD_r1_r2(3, 2); break;
        case 0x52: LD_r1_r2(3, 3); break;
        case 0x53: LD_r1_r2(3, 4); break;
        case 0x54: LD_r1_r2(3, 5); break;
        case 0x55: LD_r1_r2(3, 6); break;
        case 0x57: LD_r1_r2(3, 0); break;
        case 0x58: LD_r1_r2(4, 1); break;
        case 0x59: LD_r1_r2(4, 2); break;
        case 0x5a: LD_r1_r2(4, 3); break;
        case 0x5b: LD_r1_r2(4, 4); break;
        case 0x5c: LD_r1_r2(4, 5); break;
        case 0x5d: LD_r1_r2(4, 6); break;
        case 0x5f: LD_r1_r2(4, 0); break;
        case 0x60: LD_r1_r2(5, 1); break;
        case 0x61: LD_r1_r2(5, 2); break;
        case 0x62: LD_r1_r2(5, 3); break;
        case 0x63: LD_r1_r2(5, 4); break;
        case 0x64: LD_r1_r2(5, 5); break;
        case 0x65: LD_r1_r2(5, 6); break;
        case 0x67: LD_r1_r2(5, 0); break;
        case 0x68: LD_r1_r2(6, 1); break;
        case 0x69: LD_r1_r2(6, 2); break;
        case 0x6a: LD_r1_r2(6, 3); break;
        case 0x6b: LD_r1_r2(6, 4); break;
        case 0x6c: LD_r1_r2(6, 5); break;
        case 0x6d: LD_r1_r2(6, 6); break;
        case 0x6f: LD_r1_r2(6, 0); break;
        case 0x78: LD_r1_r2(0, 1); break;
        case 0x79: LD_r1_r2(0, 2); break;
        case 0x7a: LD_r1_r2(0, 3); break;
        case 0x7b: LD_r1_r2(0, 4); break;
        case 0x7c: LD_r1_r2(0, 5); break;
        case 0x7d: LD_r1_r2(0, 6); break;
        case 0x7f: LD_r1_r2(0, 0); break;
        case 0x0a: LD_r_pair(0, 1); break;
        case 0x1a: LD_r_pair(0, 2); break;
        case 0x2a: LD_r_pair(0, 3, false, true); break;
        case 0x3a: LD_r_pair(0, 3, true, false); break;
        case 0x46: LD_r_pair(1, 3); break;
        case 0x6e: LD_r_pair(6, 3); break;
        case 0x4e: LD_r_pair(2, 3); break;
        case 0x56: LD_r_pair(3, 3); break;
        case 0x5e: LD_r_pair(4, 3); break;
        case 0x66: LD_r_pair(5, 3); break;
        case 0x7e: LD_r_pair(0, 3); break;
        case 0x02: LD_pair_r(1, 0); break;
        case 0x12: LD_pair_r(2, 0); break;
        case 0x22: LD_pair_r(3, 0, false, true); break;
        case 0x32: LD_pair_r(3, 0, true, false); break;
        case 0x70: LD_pair_r(3, 1); break;
        case 0x71: LD_pair_r(3, 2); break;
        case 0x72: LD_pair_r(3, 3); break;
        case 0x73: LD_pair_r(3, 4); break;
        case 0x74: LD_pair_r(3, 5); break;
        case 0x75: LD_pair_r(3, 6); break;
        case 0x77: LD_pair_r(3, 0); break;
        case 0xea: LD_A_nn(true); break;
        case 0xfa: LD_A_nn(false); break;
        case 0xe2: LD_A_C(true); break;
        case 0xf2: LD_A_C(false); break;
        case 0xe0: LDH_A_n(true); break;
        case 0xf0: LDH_A_n(false); break;
        case 0x01: LD_pair_Im(1); break;
        case 0x11: LD_pair_Im(2); break;
        case 0x21: LD_pair_Im(3); break;
        case 0x31: LD_pair_Im(4); break;
        case 0xf9: LD_SP_HL(); break;
        case 0xf8: LDHL_SP_n(); break;
        case 0x08: LD_nn_SP(); break;
        case 0x36: LD_HL_Im(); break;

        case 0xf1: POP(0); break;
        case 0xd1: POP(2); break;
        case 0xc1: POP(1); break;
        case 0xe1: POP(3); break;
        case 0xc5: PUSH(1); break;
        case 0xd5: PUSH(2); break;
        case 0xf5: PUSH(0); break;
        case 0xe5: PUSH(3); break;

        case 0x80: ADD(1, 0, 0); break;
        case 0x81: ADD(2, 0, 0); break;
        case 0x82: ADD(3, 0, 0); break;
        case 0x83: ADD(4, 0, 0); break;
        case 0x84: ADD(5, 0, 0); break;
        case 0x85: ADD(6, 0, 0); break;
        case 0x86: ADD(0, 1, 0); break;
        case 0x87: ADD(0, 0, 0); break;
        case 0x88: ADD(1, 0, 1); break;
        case 0x89: ADD(2, 0, 1); break;
        case 0x8a: ADD(3, 0, 1); break;
        case 0x8b: ADD(4, 0, 1); break;
        case 0x8c: ADD(5, 0, 1); break;
        case 0x8d: ADD(6, 0, 1); break;
        case 0x8e: ADD(0, 1, 1); break;
        case 0x8f: ADD(0, 0, 1); break;
        case 0x09: ADD_HL(1); break;
        case 0x19: ADD_HL(2); break;
        case 0x29: ADD_HL(3); break;
        case 0x39: ADD_HL(4); break;
        case 0xe8: ADD_SP(); break;
        case 0xc6: ADD(0, 2, 0); break;
        case 0xce: ADD(0, 2, 1); break;
        case 0x90: SUB(1, 0, 0); break;
        case 0x91: SUB(2, 0, 0); break;
        case 0x92: SUB(3, 0, 0); break;
        case 0x93: SUB(4, 0, 0); break;
        case 0x94: SUB(5, 0, 0); break;
        case 0x95: SUB(6, 0, 0); break;
        case 0x96: SUB(0, 1, 0); break;
        case 0x98: SUB(1, 0, 1); break;
        case 0x99: SUB(2, 0, 1); break;
        case 0x9a: SUB(3, 0, 1); break;
        case 0x9b: SUB(4, 0, 1); break;
        case 0x9c: SUB(5, 0, 1); break;
        case 0x9d: SUB(6, 0, 1); break;
        case 0x9e: SUB(0, 1, 1); break;
        case 0x9f: SUB(0, 0, 1); break;
        case 0xde: SUB(0, 0, 1); break; // sbc
        case 0x97: SUB(0, 0, 0); break;
        case 0xb8: SUB(1, 0, 0, 1); break;
        case 0xb9: SUB(2, 0, 0, 1); break;
        case 0xba: SUB(3, 0, 0, 1); break;
        case 0xbb: SUB(4, 0, 0, 1); break;
        case 0xbc: SUB(5, 0, 0, 1); break;
        case 0xbd: SUB(6, 0, 0, 1); break;
        case 0xbe: SUB(0, 1, 0, 1); break;
        case 0xbf: SUB(0, 0, 0, 1); break;
        case 0xfe: SUB(0, 2, 0, 1); break;
        case 0xd6: SUB(0, 2, 0); break;
        case 0xa0: AND(1, 0); break;
        case 0xa1: AND(2, 0); break;
        case 0xa2: AND(3, 0); break;
        case 0xa3: AND(4, 0); break;
        case 0xa4: AND(5, 0); break;
        case 0xa5: AND(6, 0); break;
        case 0xa6: AND(0, 1); break;
        case 0xa7: AND(0, 0); break;
        case 0xe6: AND(0, 2); break;
        case 0xee: XOR(0, 2); break;
        case 0xa8: XOR(1, 0); break;
        case 0xa9: XOR(2, 0); break;
        case 0xaa: XOR(3, 0); break;
        case 0xab: XOR(4, 0); break;
        case 0xac: XOR(5, 0); break;
        case 0xad: XOR(6, 0); break;
        case 0xae: XOR(0, 1); break;
        case 0xaf: XOR(0, 0); break;
        case 0xb0: OR(1, 0); break;
        case 0xb1: OR(2, 0); break;
        case 0xb2: OR(3, 0); break;
        case 0xb3: OR(4, 0); break;
        case 0xb4: OR(5, 0); break;
        case 0xb5: OR(6, 0); break;
        case 0xb6: OR(0, 1); break;
        case 0xb7: OR(0, 0); break;
        case 0xf6: OR(0, 2); break;
        case 0x3c: INC(0); break;
        case 0x04: INC(1); break;
        case 0x0c: INC(2); break;
        case 0x14: INC(3); break;
        case 0x1c: INC(4); break;
        case 0x24: INC(5); break;
        case 0x2c: INC(6); break;
        case 0x34: INC(0, 1); break;
        case 0x05: DEC(1); break;
        case 0x0d: DEC(2); break;
        case 0x15: DEC(3); break;
        case 0x25: DEC(5); break;
        case 0x2d: DEC(6); break;
        case 0x3d: DEC(0); break;
        case 0x1d: DEC(4); break;
        case 0x35: DEC(0, 1); break;
        case 0x03: INC_16(1); break;
        case 0x13: INC_16(2); break;
        case 0x23: INC_16(3); break;
        case 0x33: INC_16(4); break;
        case 0x2b: DEC_16(3); break;
        case 0x3b: DEC_16(4); break;
        case 0x1b: DEC_16(2); break;
        case 0x0b: DEC_16(1); break;

        case 0xd9: RETI(); break;
        case 0xe9: JP_HL(); break;
        case 0x07: RLCA(); break;
        case 0x0f: RRCA(); break;
        case 0x10: STOP(); break;
        case 0x17: RLA(); break;
        case 0x1f: RRA(); break;
        case 0x27: DAA(); break;
        case 0x2f: CPL(); break;
        case 0x37: SCF(); break;
        case 0x3f: CCF(); break;
        case 0xcb: execute_extended_opcode(); break;
        case 0xf3: DI(); break;
        case 0xfb: EI(); break;
        case 0x76: HALT(); break;

        case 0x18: JR(1); break;
        case 0x20: JR(flags.cc.z == 0); break;
        case 0x28: JR(flags.cc.z == 1); break;
        case 0x30: JR(flags.cc.c == 0); break;
        case 0x38: JR(flags.cc.c == 1); break;
        case 0xc3: JP(1); break;
        case 0xc2: JP(flags.cc.z == 0); break;
        case 0xca: JP(flags.cc.z == 1); break;
        case 0xd2: JP(flags.cc.c == 0); break;
        case 0xda: JP(flags.cc.c == 1); break;
        case 0xcd: CALL(1); break;
        case 0xc4: CALL(flags.cc.z == 0); break;
        case 0xd4: CALL(flags.cc.c == 0); break;
        case 0xcc: CALL(flags.cc.z == 1); break;
        case 0xdc: CALL(flags.cc.c == 1); break;
        case 0xc9: RET(1); break;
        case 0xc0: RET(flags.cc.z == 0); break;
        case 0xc8: RET(flags.cc.z == 1); break;
        case 0xd0: RET(flags.cc.c == 0); break;
        case 0xd8: RET(flags.cc.c == 1); break;
        case 0xf7: RST(0x30); break;
        case 0xe7: RST(0x20); break;
        case 0xef: RST(0x28); break;
        case 0xdf: RST(0x18); break;
        case 0xd7: RST(0x10); break;
        case 0xcf: RST(0x08); break;
        case 0xc7: RST(0x00); break;
        case 0xff: RST(0x38); break;





        case 0xd3: UNIMPLEMENTED_INSTRUCTION(); break;
        case 0xdb: UNIMPLEMENTED_INSTRUCTION(); break;
        case 0xdd: UNIMPLEMENTED_INSTRUCTION(); break;
        case 0xe3: UNIMPLEMENTED_INSTRUCTION(); break;
        case 0xe4: UNIMPLEMENTED_INSTRUCTION(); break;
        case 0xeb: UNIMPLEMENTED_INSTRUCTION(); break;
        case 0xec: UNIMPLEMENTED_INSTRUCTION(); break;
        case 0xed: UNIMPLEMENTED_INSTRUCTION(); break;
        case 0xf4: UNIMPLEMENTED_INSTRUCTION(); break;
        case 0xfc: UNIMPLEMENTED_INSTRUCTION(); break;
        case 0xfd: UNIMPLEMENTED_INSTRUCTION(); break;
        }
        return opcode;
    }
    void cb_SWAP(int reg, bool HL = false)
    {
        u8 value, answer;
        if (HL) value = rb(hl());
        else value = regs[reg];

        u8 lower_nib = value & 0x0f;
        u8 upper_nib = value & 0xf0;
        answer = (upper_nib >> 4) | (lower_nib << 4);
        flags.cc.z = answer == 0;
        flags.cc.c = flags.cc.n = flags.cc.h = 0;

        if (HL)
        {
            wb(hl(), answer);
            system_cycles += 16;
        }
        else
        {
            regs[reg] = answer;
            system_cycles += 8;
        }
    }
    void cb_RLC(int reg, bool HL = false)
    {
        u8 value, answer;
        if (HL) value = rb(hl());
        else value = regs[reg];

        u8 old_bit7 = (value >> 7) & 0x1;
        answer = (value << 1) | old_bit7;
        flags.cc.c = old_bit7;
        flags.cc.n = 0;
        flags.cc.h = 0;
        flags.cc.z = answer == 0 ? 1 : 0;

        if (HL)
        {
            wb(hl(), answer);
            system_cycles += 16;
        }
        else
        {
            regs[reg] = answer;
            system_cycles += 8;
        }
    }
    void cb_RL(int reg, bool HL = false)
    {
        u8 value, answer;
        if (HL) value = rb(hl());
        else value = regs[reg];

        u8 old_bit7 = (value >> 7);
        answer = (value << 1) | flags.cc.c;
        flags.cc.c = old_bit7;
        flags.cc.n = 0;
        flags.cc.h = 0;
        flags.cc.z = answer == 0 ? 1 : 0;

        if (HL)
        {
            wb(hl(), answer);
            system_cycles += 16;
        }
        else
        {
            regs[reg] = answer;
            system_cycles += 8;
        }
    }
    void cb_RRC(int reg, bool HL = false)
    {
        u8 value, answer;
        if (HL) value = rb(hl());
        else value = regs[reg];

        u8 old_bit0 = value & 0x1;
        answer = (value >> 1) | (old_bit0 << 7);
        flags.cc.c = old_bit0;
        flags.cc.n = 0;
        flags.cc.h = 0;
        flags.cc.z = answer == 0 ? 1 : 0;

        if (HL)
        {
            wb(hl(), answer);
            system_cycles += 16;
        }
        else
        {
            regs[reg] = answer;
            system_cycles += 8;
        }
    }
    void cb_RR(int reg, bool HL = false)
    {
        u8 value, answer;
        if (HL) value = rb(hl());
        else value = regs[reg];

        u8 old_bit0 = value & 0x1;
        answer = (value >> 1) | (((u8)flags.cc.c) << 7);
        flags.cc.c = old_bit0;
        flags.cc.n = 0;
        flags.cc.h = 0;
        flags.cc.z = answer == 0 ? 1 : 0;

        if (HL)
        {
            wb(hl(), answer);
            system_cycles += 16;
        }
        else
        {
            regs[reg] = answer;
            system_cycles += 8;
        }
    }
    void cb_SLA(int reg, bool HL = false)
    {
        u8 value, answer;
        if (HL) value = rb(hl());
        else value = regs[reg];

        u8 old_bit7 = (value >> 7);
        answer = value << 1;
        flags.cc.c = old_bit7;
        flags.cc.n = 0;
        flags.cc.h = 0;
        flags.cc.z = answer == 0 ? 1 : 0;

        if (HL)
        {
            wb(hl(), answer);
            system_cycles += 16;
        }
        else
        {
            regs[reg] = answer;
            system_cycles += 8;
        }
    }
    void cb_SRA(int reg, bool HL = false)
    {
        u8 value, answer;
        if (HL) value = rb(hl());
        else value = regs[reg];

        u8 old_bit0 = value & 0x1;
        u8 msb = value & 0x80;
        answer = (value >> 1) | msb;
        flags.cc.c = old_bit0;
        flags.cc.n = 0;
        flags.cc.h = 0;
        flags.cc.z = answer == 0 ? 1 : 0;

        if (HL)
        {
            wb(hl(), answer);
            system_cycles += 16;
        }
        else
        {
            regs[reg] = answer;
            system_cycles += 8;
        }
    }
    void cb_SRL(int reg, bool HL = false)
    {
        u8 value, answer;
        if (HL) value = rb(hl());
        else value = regs[reg];

        u8 old_bit0 = value & 0x1;
        answer = value >> 1;
        flags.cc.c = old_bit0;
        flags.cc.n = 0;
        flags.cc.h = 0;
        flags.cc.z = answer == 0 ? 1 : 0;

        if (HL)
        {
            wb(hl(), answer);
            system_cycles += 16;
        }
        else
        {
            regs[reg] = answer;
            system_cycles += 8;
        }
    }
    void cb_BIT(int bit, int reg, bool HL = false)
    {
        u8 value;
        if (HL) value = rb(hl());
        else value = regs[reg];

        value = (value >> bit) & 0x1;

        flags.cc.z = value == 0 ? 1 : 0;
        flags.cc.n = 0;
        flags.cc.h = 1;
    }
    void cb_RESET(int bit, int reg, bool HL = false)
    {
        u8 value, answer;
        if (HL) value = rb(hl());
        else value = regs[reg];

        answer = value & ~(0x1 << bit);

        if (HL)
        {
            wb(hl(), answer);
            system_cycles += 16;
        }
        else
        {
            regs[reg] = answer;
            system_cycles += 8;
        }
    }
    void cb_SET(int bit, int reg, bool HL = false)
    {
        u8 value, answer;
        if (HL) value = rb(hl());
        else value = regs[reg];

        answer = value | (0x1 << bit);

        if (HL)
        {
            wb(hl(), answer);
            system_cycles += 16;
        }
        else
        {
            regs[reg] = answer;
            system_cycles += 8;
        }
    }
    void execute_extended_opcode()
    {
        u8 opcode = rb(pc);
        pc++;
        switch (opcode)
        {
        case 0x00: cb_RLC(1, false); break;
        case 0x01: cb_RLC(2, false); break;
        case 0x02: cb_RLC(3, false); break;
        case 0x03: cb_RLC(4, false); break;
        case 0x04: cb_RLC(5, false); break;
        case 0x05: cb_RLC(6, false); break;
        case 0x06: cb_RLC(0, true); break;
        case 0x07: cb_RLC(0, false); break;
        case 0x08: cb_RRC(1, false); break;
        case 0x09: cb_RRC(2, false); break;
        case 0x0a: cb_RRC(3, false); break;
        case 0x0b: cb_RRC(4, false); break;
        case 0x0c: cb_RRC(5, false); break;
        case 0x0d: cb_RRC(6, false); break;
        case 0x0e: cb_RRC(0, true); break;
        case 0x0f: cb_RRC(0, false); break;
        case 0x10: cb_RL(1, false); break;
        case 0x11: cb_RL(2, false); break;
        case 0x12: cb_RL(3, false); break;
        case 0x13: cb_RL(4, false); break;
        case 0x14: cb_RL(5, false); break;
        case 0x15: cb_RL(6, false); break;
        case 0x16: cb_RL(0, true); break;
        case 0x17: cb_RL(0, false); break;
        case 0x18: cb_RR(1, false); break;
        case 0x19: cb_RR(2, false); break;
        case 0x1a: cb_RR(3, false); break;
        case 0x1b: cb_RR(4, false); break;
        case 0x1c: cb_RR(5, false); break;
        case 0x1d: cb_RR(6, false); break;
        case 0x1e: cb_RR(0, true); break;
        case 0x1f: cb_RR(0, false); break;
        case 0x20: cb_SLA(1, false); break;
        case 0x21: cb_SLA(2, false); break;
        case 0x22: cb_SLA(3, false); break;
        case 0x23: cb_SLA(4, false); break;
        case 0x24: cb_SLA(5, false); break;
        case 0x25: cb_SLA(6, false); break;
        case 0x26: cb_SLA(0, true); break;
        case 0x27: cb_SLA(0, false); break;
        case 0x28: cb_SRA(1, false); break;
        case 0x29: cb_SRA(2, false); break;
        case 0x2a: cb_SRA(3, false); break;
        case 0x2b: cb_SRA(4, false); break;
        case 0x2c: cb_SRA(5, false); break;
        case 0x2d: cb_SRA(6, false); break;
        case 0x2e: cb_SRA(0, true); break;
        case 0x2f: cb_SRA(0, false); break;
        case 0x30: cb_SWAP(1, false); break;
        case 0x31: cb_SWAP(2, false); break;
        case 0x32: cb_SWAP(3, false); break;
        case 0x33: cb_SWAP(4, false); break;
        case 0x34: cb_SWAP(5, false); break;
        case 0x35: cb_SWAP(6, false); break;
        case 0x36: cb_SWAP(0, true); break;
        case 0x37: cb_SWAP(0, false);  break;
        case 0x38: cb_SRL(1, false); break;
        case 0x39: cb_SRL(2, false); break;
        case 0x3a: cb_SRL(3, false); break;
        case 0x3b: cb_SRL(4, false); break;
        case 0x3c: cb_SRL(5, false); break;
        case 0x3d: cb_SRL(6, false); break;
        case 0x3e: cb_SRL(0, true); break;
        case 0x3f: cb_SRL(0, false); break;
        case 0x40: cb_BIT(0, 1, false); break;
        case 0x41: cb_BIT(0, 2, false); break;
        case 0x42: cb_BIT(0, 3, false); break;
        case 0x43: cb_BIT(0, 4, false); break;
        case 0x44: cb_BIT(0, 5, false); break;
        case 0x45: cb_BIT(0, 6, false); break;
        case 0x46: cb_BIT(0, 0, true); break;
        case 0x47: cb_BIT(0, 0, false); break;
        case 0x48: cb_BIT(1, 1, false); break;
        case 0x49: cb_BIT(1, 2, false); break;
        case 0x4a: cb_BIT(1, 3, false); break;
        case 0x4b: cb_BIT(1, 4, false); break;
        case 0x4c: cb_BIT(1, 5, false); break;
        case 0x4d: cb_BIT(1, 6, false); break;
        case 0x4e: cb_BIT(1, 0, true); break;
        case 0x4f: cb_BIT(1, 0, false); break;
        case 0x50: cb_BIT(2, 1, false); break;
        case 0x51: cb_BIT(2, 2, false); break;
        case 0x52: cb_BIT(2, 3, false); break;
        case 0x53: cb_BIT(2, 4, false); break;
        case 0x54: cb_BIT(2, 5, false); break;
        case 0x55: cb_BIT(2, 6, false); break;
        case 0x56: cb_BIT(2, 0, true); break;
        case 0x57: cb_BIT(2, 0, false); break;
        case 0x58: cb_BIT(3, 1, false); break;
        case 0x59: cb_BIT(3, 2, false); break;
        case 0x5a: cb_BIT(3, 3, false); break;
        case 0x5b: cb_BIT(3, 4, false); break;
        case 0x5c: cb_BIT(3, 5, false); break;
        case 0x5d: cb_BIT(3, 6, false); break;
        case 0x5e: cb_BIT(3, 0, true); break;
        case 0x5f: cb_BIT(3, 0, false); break;
        case 0x60: cb_BIT(4, 1, false); break;
        case 0x61: cb_BIT(4, 2, false); break;
        case 0x62: cb_BIT(4, 3, false); break;
        case 0x63: cb_BIT(4, 4, false); break;
        case 0x64: cb_BIT(4, 5, false); break;
        case 0x65: cb_BIT(4, 6, false); break;
        case 0x66: cb_BIT(4, 0, true); break;
        case 0x67: cb_BIT(4, 0, false); break;
        case 0x68: cb_BIT(5, 1, false); break;
        case 0x69: cb_BIT(5, 2, false); break;
        case 0x6a: cb_BIT(5, 3, false); break;
        case 0x6b: cb_BIT(5, 4, false); break;
        case 0x6c: cb_BIT(5, 5, false); break;
        case 0x6d: cb_BIT(5, 6, false); break;
        case 0x6e: cb_BIT(5, 0, true); break;
        case 0x6f: cb_BIT(5, 0, false); break;
        case 0x70: cb_BIT(6, 1, false); break;
        case 0x71: cb_BIT(6, 2, false); break;
        case 0x72: cb_BIT(6, 3, false); break;
        case 0x73: cb_BIT(6, 4, false); break;
        case 0x74: cb_BIT(6, 5, false); break;
        case 0x75: cb_BIT(6, 6, false); break;
        case 0x76: cb_BIT(6, 0, true); break;
        case 0x77: cb_BIT(6, 0, false); break;
        case 0x78: cb_BIT(7, 1, false); break;
        case 0x79: cb_BIT(7, 2, false); break;
        case 0x7a: cb_BIT(7, 3, false); break;
        case 0x7b: cb_BIT(7, 4, false); break;
        case 0x7c: cb_BIT(7, 5, false); break;
        case 0x7d: cb_BIT(7, 6, false); break;
        case 0x7e: cb_BIT(7, 0, true); break;
        case 0x7f: cb_BIT(7, 0, false); break;
        case 0x80: cb_RESET(0, 1, false); break;
        case 0x81: cb_RESET(0, 2, false); break;
        case 0x82: cb_RESET(0, 3, false); break;
        case 0x83: cb_RESET(0, 4, false); break;
        case 0x84: cb_RESET(0, 5, false); break;
        case 0x85: cb_RESET(0, 6, false); break;
        case 0x86: cb_RESET(0, 0, true); break;
        case 0x87: cb_RESET(0, 0, false); break;
        case 0x88: cb_RESET(1, 1, false); break;
        case 0x89: cb_RESET(1, 2, false); break;
        case 0x8a: cb_RESET(1, 3, false); break;
        case 0x8b: cb_RESET(1, 4, false); break;
        case 0x8c: cb_RESET(1, 5, false); break;
        case 0x8d: cb_RESET(1, 6, false); break;
        case 0x8e: cb_RESET(1, 0, true); break;
        case 0x8f: cb_RESET(1, 0, false); break;
        case 0x90: cb_RESET(2, 1, false); break;
        case 0x91: cb_RESET(2, 2, false); break;
        case 0x92: cb_RESET(2, 3, false); break;
        case 0x93: cb_RESET(2, 4, false); break;
        case 0x94: cb_RESET(2, 5, false); break;
        case 0x95: cb_RESET(2, 6, false); break;
        case 0x96: cb_RESET(2, 0, true); break;
        case 0x97: cb_RESET(2, 0, false); break;
        case 0x98: cb_RESET(3, 1, false); break;
        case 0x99: cb_RESET(3, 2, false); break;
        case 0x9a: cb_RESET(3, 3, false); break;
        case 0x9b: cb_RESET(3, 4, false); break;
        case 0x9c: cb_RESET(3, 5, false); break;
        case 0x9d: cb_RESET(3, 6, false); break;
        case 0x9e: cb_RESET(3, 0, true); break;
        case 0x9f: cb_RESET(3, 0, false); break;
        case 0xa0: cb_RESET(4, 1, false); break;
        case 0xa1: cb_RESET(4, 2, false); break;
        case 0xa2: cb_RESET(4, 3, false); break;
        case 0xa3: cb_RESET(4, 4, false); break;
        case 0xa4: cb_RESET(4, 5, false); break;
        case 0xa5: cb_RESET(4, 6, false); break;
        case 0xa6: cb_RESET(4, 0, true); break;
        case 0xa7: cb_RESET(4, 0, false); break;
        case 0xa8: cb_RESET(5, 1, false); break;
        case 0xa9: cb_RESET(5, 2, false); break;
        case 0xaa: cb_RESET(5, 3, false); break;
        case 0xab: cb_RESET(5, 4, false); break;
        case 0xac: cb_RESET(5, 5, false); break;
        case 0xad: cb_RESET(5, 6, false); break;
        case 0xae: cb_RESET(5, 0, true); break;
        case 0xaf: cb_RESET(5, 0, false); break;
        case 0xb0: cb_RESET(6, 1, false); break;
        case 0xb1: cb_RESET(6, 2, false); break;
        case 0xb2: cb_RESET(6, 3, false); break;
        case 0xb3: cb_RESET(6, 4, false); break;
        case 0xb4: cb_RESET(6, 5, false); break;
        case 0xb5: cb_RESET(6, 6, false); break;
        case 0xb6: cb_RESET(6, 0, true); break;
        case 0xb7: cb_RESET(6, 0, false); break;
        case 0xb8: cb_RESET(7, 1, false); break;
        case 0xb9: cb_RESET(7, 2, false); break;
        case 0xba: cb_RESET(7, 3, false); break;
        case 0xbb: cb_RESET(7, 4, false); break;
        case 0xbc: cb_RESET(7, 5, false); break;
        case 0xbd: cb_RESET(7, 6, false); break;
        case 0xbe: cb_RESET(7, 0, true); break;
        case 0xbf: cb_RESET(7, 0, false); break;
        case 0xc0: cb_SET(0, 1, false); break;
        case 0xc1: cb_SET(0, 2, false); break;
        case 0xc2: cb_SET(0, 3, false); break;
        case 0xc3: cb_SET(0, 4, false); break;
        case 0xc4: cb_SET(0, 5, false); break;
        case 0xc5: cb_SET(0, 6, false); break;
        case 0xc6: cb_SET(0, 0, true); break;
        case 0xc7: cb_SET(0, 0, false); break;
        case 0xc8: cb_SET(1, 1, false); break;
        case 0xc9: cb_SET(1, 2, false); break;
        case 0xca: cb_SET(1, 3, false); break;
        case 0xcb: cb_SET(1, 4, false); break;
        case 0xcc: cb_SET(1, 5, false); break;
        case 0xcd: cb_SET(1, 6, false); break;
        case 0xce: cb_SET(1, 0, true); break;
        case 0xcf: cb_SET(1, 0, false); break;
        case 0xd0: cb_SET(2, 1, false); break;
        case 0xd1: cb_SET(2, 2, false); break;
        case 0xd2: cb_SET(2, 3, false); break;
        case 0xd3: cb_SET(2, 4, false); break;
        case 0xd4: cb_SET(2, 5, false); break;
        case 0xd5: cb_SET(2, 6, false); break;
        case 0xd6: cb_SET(2, 0, true); break;
        case 0xd7: cb_SET(2, 0, false); break;
        case 0xd8: cb_SET(3, 1, false); break;
        case 0xd9: cb_SET(3, 2, false); break;
        case 0xda: cb_SET(3, 3, false); break;
        case 0xdb: cb_SET(3, 4, false); break;
        case 0xdc: cb_SET(3, 5, false); break;
        case 0xdd: cb_SET(3, 6, false); break;
        case 0xde: cb_SET(3, 0, true); break;
        case 0xdf: cb_SET(3, 0, false); break;
        case 0xe0: cb_SET(4, 1, false); break;
        case 0xe1: cb_SET(4, 2, false); break;
        case 0xe2: cb_SET(4, 3, false); break;
        case 0xe3: cb_SET(4, 4, false); break;
        case 0xe4: cb_SET(4, 5, false); break;
        case 0xe5: cb_SET(4, 6, false); break;
        case 0xe6: cb_SET(4, 0, true); break;
        case 0xe7: cb_SET(4, 0, false); break;
        case 0xe8: cb_SET(5, 1, false); break;
        case 0xe9: cb_SET(5, 2, false); break;
        case 0xea: cb_SET(5, 3, false); break;
        case 0xeb: cb_SET(5, 4, false); break;
        case 0xec: cb_SET(5, 5, false); break;
        case 0xed: cb_SET(5, 6, false); break;
        case 0xee: cb_SET(5, 0, true); break;
        case 0xef: cb_SET(5, 0, false); break;
        case 0xf0: cb_SET(6, 1, false); break;
        case 0xf1: cb_SET(6, 2, false); break;
        case 0xf2: cb_SET(6, 3, false); break;
        case 0xf3: cb_SET(6, 4, false); break;
        case 0xf4: cb_SET(6, 5, false); break;
        case 0xf5: cb_SET(6, 6, false); break;
        case 0xf6: cb_SET(6, 0, true); break;
        case 0xf7: cb_SET(6, 0, false); break;
        case 0xf8: cb_SET(7, 1, false); break;
        case 0xf9: cb_SET(7, 2, false); break;
        case 0xfa: cb_SET(7, 3, false); break;
        case 0xfb: cb_SET(7, 4, false); break;
        case 0xfc: cb_SET(7, 5, false); break;
        case 0xfd: cb_SET(7, 6, false); break;
        case 0xfe: cb_SET(7, 0, true); break;
        case 0xff: cb_SET(7, 0, false); break;
        }
    }
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
    int system_rom_bank_number  = 1;
    int system_eram_bank_number = 0;


    Emulator(const char*);
    void run_step();
    void run_frame();
    void key_pressed(int);
    void key_released(int);
    int  update_gpu(int);
    void update_interrupts();
    void update_timers(int);
    void wb(uint16_t, u8);
    u8 rb(uint16_t) const;


    int  system_cycles              = 0;
    bool system_is_halted           = false;
    bool system_master_interrupt_en = true;
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
    void gpu_generate_background()
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
                u8 tile_ref;
                if (get_bit(reg_FF40_LCD_ctrl, 3) == 0) // which bg tile map 
                    tile_ref = mem_vram[0x1800 + tile_idx];
                else
                    tile_ref = mem_vram[0x1C00 + tile_idx];

                // get the pixel from the tile depending on which tile set we're in 
                if (get_bit(reg_FF40_LCD_ctrl, 4) == 0)
                    gpu_background[y][x] = system_bg_palette[gpu_tileset[128 + tile_ref][y & 0b111][x & 0b111]]; // TODO: this is wrong?
                else
                    gpu_background[y][x] = system_bg_palette[gpu_tileset[tile_ref][y & 0b111][x & 0b111]];
            }
        }
    }
    void gpu_generate_tileset_view()
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
    void gpu_update_tileset_from_addr(uint16_t addr) // address is between 0x8000 - 0x97FF
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
            if (color > 3) printf("SOY %i\n", color);
            gpu_tileset[idx_in_tileset][row][j] = color;
        }
    }
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
    void gpu_construct_oam_entries()
    {
        // iterate over the 40 entries of 4 bytes each starting at FE00
        const int oam_start = 0xFE00;
        for (int i = 0; i < 40; i++)
        {
            gpu_oam_entries[i].y_coord = rb(oam_start + i * 4 + 0);
            gpu_oam_entries[i].x_coord = rb(oam_start + i * 4 + 1);
            gpu_oam_entries[i].tile_ref = rb(oam_start + i * 4 + 2);
            gpu_oam_entries[i].data = rb(oam_start + i * 4 + 3);
        }
    }
    void gpu_update_oam_entries(uint16_t addr, u8 data) // assume addr is FE00 - FE9F
    {
        int entry_idx = (addr - 0xFE00) >> 2; // since stride is 4 bytes 
        switch (addr & 0b11) // which of the 4 bytes do we need to modify?
        {
        case 0:gpu_oam_entries[entry_idx].y_coord = data; break;
        case 1:gpu_oam_entries[entry_idx].x_coord = data; break;
        case 2:gpu_oam_entries[entry_idx].tile_ref = data; break;
        case 3:gpu_oam_entries[entry_idx].data = data; break;
        }
    }
};

u8 Emulator::rb(uint16_t addr) const
{
    switch (addr & 0xF000)
    {
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000:
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000: return mem_cart[addr]; // ROM0, ROM1 (unbanked) (32k)
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
    switch (addr & 0xF000)
    {
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000:
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000: return; // ROM0, ROM1 (unbanked) (32k)
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
                gpu_update_oam_entries(addr, data);
                return;
            }
        case 0xF00:
            if (addr <= 0xFF7F) // I/O registers
            {
                switch (addr)
                {
                case 0xFF00: reg_FF00_joypad         = data & 0x3F; return; // Joypad reg, only bits 4,5 write-able
                case 0xFF04: reg_FF04_div            = data;  return;
                case 0xFF05: reg_FF05_tima           = data; return;
                case 0xFF06: reg_FF06_tma            = data;  return;
                case 0xFF07: reg_FF07_tac            = data;  return;
                case 0xFF0F: reg_FF0F_interrupt_flag = data; return;
                case 0xFF40: reg_FF40_LCD_ctrl       = data; return;
                case 0xFF41: reg_FF41_LCD_stat       = data; return;
                case 0xFF42: reg_FF42_scroll_Y       = data; return; 
                case 0xFF43: reg_FF43_scroll_X       = data; return; 
                case 0xFF44: reg_FF44_lineY          = data; return;// Current scanline, should not happen!
                case 0xFF45: reg_FF45_lineY_compare  = data; return;
                case 0xFF46: reg_FF46_dma            = data; return;
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






Emulator::Emulator(const char* romName)
{
    // init memory 
    for (int i = 0; i < NUMBER_OF_ERAM_BANKS; i++) mem_eram[i] = new u8[0x2000];
    for (int i = 0; i < NUMBER_OF_ERAM_BANKS; i++) memset(mem_eram[i], 0x69, sizeof(mem_eram[i]));
    memset(gpu_oam_entries    , 0, 40 * sizeof(oam_entry));
    memset(gpu_tileset    , 0, 384 * sizeof(pixels));
    memset(gpu_background , 0, 256 * sizeof(row));
    memset(gpu_screen_data    , 0, 144 * 160 * sizeof(uint32_t));
    memset(mem_cart, 0, 0x200000);
    memset(mem_vram          , 0, 0x2000);
    memset(mem_wram          , 0, 0x2000);
    memset(mem_oam           , 0, 160);
    memset(mem_hram          , 0, 128);

    // load rom 
    FILE* in = fopen(romName, "rb");
    fread(mem_cart, 1, 0x200000, in);
    fclose(in);

    // copy bios 
    //memcpy(&_game_cartridge[0], &_bios[0], 256);

    // init values in memory 
    a(0x01);
    bc(0x0013);
    de(0x00D8);
    hl(0x014D);
    flags.psw         = 0xB0;
    pc                = 0x100;
    sp                = 0xFFFE;
}

void Emulator::run_step()
{
    int _before_OP = system_cycles; // cycles before op
    execute_opcode();
    int delta = system_cycles - _before_OP; // cycles taken during op

    //update_timers(delta);
    int do_vblank = update_gpu(delta); //if (do_vblank) return; // gpu sends signal it's time to vblank so we render 
    update_interrupts();
}

void Emulator::run_frame()
{
    const int target_cycles = 70221;
    system_cycles %= 70221;
    while ((system_cycles < target_cycles))
    {
        int _before_OP    = system_cycles; // cycles before op
        execute_opcode();
        int delta         = system_cycles - _before_OP; // cycles taken during op

        //update_timers(delta);
        int do_vblank = update_gpu(delta); //if (do_vblank) return; // gpu sends signal it's time to vblank so we render 
        update_interrupts();
    }
}

void Emulator::update_timers(int _cyc)
{

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
                    reset_bit(reg_FF0F_interrupt_flag, i);

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






int Emulator::update_gpu(int _cyc) // returns true if it is time to vblank
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
        return 0;

    case GPU_STATE::SCANLINE_VRAM: // VRAM read mode, scanline active. Treat end of mode 3 as end of scanline
        if (gpu_mini_timer >= 172) // Enter hblank
        {
            gpu_mini_timer = 0;
            gpu_state = GPU_STATE::HBLANK;
            gpu_render_scanline(); // Write a scanline to the framebuffer
        }
        return 0;

    case GPU_STATE::HBLANK: // Hblank. After the last hblank, push the screen data to canvas
        if (gpu_mini_timer >= 204)
        {
            gpu_mini_timer = 0;
            reg_FF44_lineY++; // scan line reg

            if (reg_FF44_lineY == 143) // Enter vblank
            {
                gpu_state = GPU_STATE::VBLANK;
                //GPU._canvas.putImageData(GPU._scrn, 0, 0);
                return 1; // my version of a function ptr: return 1 to signal vblank
            }
            else gpu_state = GPU_STATE::SCANLINE_OAM; // continue in OAM mode for next scanline
        }
        return 0;

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
        return 0;
    }
}

void Emulator::gpu_render_scanline()
{
    int screen_offset = reg_FF44_lineY * 160;
    int screen_x = 0;

    int y = reg_FF44_lineY + reg_FF42_scroll_Y; // gpu line + scroll Y

    int background;
    for (int x = reg_FF43_scroll_X; x < reg_FF43_scroll_X + 160; x++)
    {
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
        if (get_bit(reg_FF40_LCD_ctrl, 4) == 0)
            background = gpu_tileset[128 + tile_ref][y & 0x7][x & 0x7]; // TODO: this is wrong?
        else
            background = gpu_tileset[tile_ref][y & 0x7][x & 0x7];
        
        //printf("%u\n", background);
        gpu_screen_data[screen_offset + screen_x] = system_bg_palette[background];
        screen_x++;
    }

    // GOOD TILL HERE 

    // simplified
    for (int i = 0; i < 1; i++)
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

    // full version 
    /*
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

void Emulator::key_pressed(int key)
{
    switch (key)
    {
    case 0: system_input_keys = reset_bit(system_input_keys, 0); break;
    case 1: system_input_keys = reset_bit(system_input_keys, 1); break;
    case 2: system_input_keys = reset_bit(system_input_keys, 2); break;
    case 3: system_input_keys = reset_bit(system_input_keys, 3); break;
    case 4: system_input_keys = reset_bit(system_input_keys, 4); break;
    case 5: system_input_keys = reset_bit(system_input_keys, 5); break;
    case 6: system_input_keys = reset_bit(system_input_keys, 6); break;
    case 7: system_input_keys = reset_bit(system_input_keys, 7); break;
    }
}
void Emulator::key_released(int key)
{
    switch (key)
    {
    case 0: system_input_keys = set_bit(system_input_keys, 0); break;
    case 1: system_input_keys = set_bit(system_input_keys, 1); break;
    case 2: system_input_keys = set_bit(system_input_keys, 2); break;
    case 3: system_input_keys = set_bit(system_input_keys, 3); break;
    case 4: system_input_keys = set_bit(system_input_keys, 4); break;
    case 5: system_input_keys = set_bit(system_input_keys, 5); break;
    case 6: system_input_keys = set_bit(system_input_keys, 6); break;
    case 7: system_input_keys = set_bit(system_input_keys, 7); break;
    }
}

