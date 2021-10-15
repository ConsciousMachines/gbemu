// SP always pointing at the last element
// PC always pointing at the next instruction
#include "Emulator.h"


static void UNIMPLEMENTED_INSTRUCTION() { printf("UNIMPLEMENTED_INSTRUCTION\n"); }


void Emulator::print_state() const
{
    printf(" c h n z \tA\tB\tC\tD\tE\tH\tL\tSP\tPC\n %c %c %c %c \t%02x\t%02x\t%02x\t%02x\t%02x\t%02x\t%02x\t%04x\t%04x\t\n\n",
        flags.cc.c == 1 ? 'x' : '.', flags.cc.h == 1 ? 'x' : '.', flags.cc.n == 1 ? 'x' : '.', flags.cc.z == 1 ? 'x' : '.',
        regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], sp, pc);
}
void Emulator::push_byte_4t(u8 b1)
{
    sp--;
    wb(sp, b1, true);
}
u8 Emulator::pop_byte_4t()
{
    u8 ret = rb(sp, true);
    sp++;
    return ret;
}
void Emulator::push_word_8t(uint16_t w1, bool MSB_highest)
{
    if (MSB_highest)
    {
        push_byte_4t(w1 >> 8);
        push_byte_4t(((u8)w1));
    }
    else
    {
        push_byte_4t(((u8)w1));
        push_byte_4t(w1 >> 8);
    }
}
uint16_t Emulator::pop_word_8t(bool MSB_highest)
{
    u8 b1 = pop_byte_4t();
    u8 b2 = pop_byte_4t();
    if (MSB_highest) return (((uint16_t)b2) << 8) | ((uint16_t)b1);
    else return (((uint16_t)b1) << 8) | ((uint16_t)b2);
}
u8 Emulator::next_byte_4t()
{
    u8 ret = rb(pc, true);
    pc++;
    return ret;
}
uint16_t Emulator::next_word_8t(bool LSB_first)
{
    u8 b1 = next_byte_4t();
    u8 b2 = next_byte_4t();
    if (LSB_first) return (((uint16_t)b2) << 8) | ((uint16_t)b1);
    else return (((uint16_t)b1) << 8) | ((uint16_t)b2);
}
void Emulator::LD_r_Im(int reg)
{
    u8 immediate = next_byte_4t();
    regs[reg] = immediate;
    //system_cycles += 8;
}
void Emulator::LD_r_r(int reg1, int reg2)
{
    regs[reg1] = regs[reg2];
    //system_cycles += 4;
}
void Emulator::LD_r_pair(int reg, int pair, bool decHL, bool incHL)
{
    u8 value = 0;
    switch (pair)
    {
    case 0: value = rb(af(), true); break;
    case 1: value = rb(bc(), true); break;
    case 2: value = rb(de(), true); break;
    case 3:
        value = rb(hl(), true);
        if (decHL) hl(hl() - 1);
        if (incHL) hl(hl() + 1);
        break;
    }
    regs[reg] = value;
    //system_cycles += 8;
}
void Emulator::LD_pair_r(int pair, int reg, bool decHL, bool incHL)
{
    switch (pair)
    {
    case 0: wb(af(), regs[reg], true); break;
    case 1: wb(bc(), regs[reg], true); break;
    case 2: wb(de(), regs[reg], true); break;
    case 3:
        wb(hl(), regs[reg], true);
        if (decHL) hl(hl() - 1);
        if (incHL) hl(hl() + 1);
        break;
    }
    //system_cycles += 8;
}
void Emulator::LD_HL_Im()
{
    wb(hl(), next_byte_4t(), true);
    //system_cycles += 12;
}
void Emulator::LD_A_nn(bool flip)
{
    uint16_t addr = next_word_8t();
    if (flip) wb(addr, regs[0], true);
    else regs[0] = rb(addr, true);
    //system_cycles += 16;
}
void Emulator::LD_A_C(bool flip)
{
    if (flip) wb(0xff00 + c(), a(), true);
    else regs[0] = rb(0xff00 + c(), true);
    //system_cycles += 8;
}
void Emulator::LDH_A_n(bool flip)
{
    u8 n = next_byte_4t();
    if (flip) wb(0xff00 + n, a(), true);
    else a(rb(0xff00 + n, true));
    //system_cycles += 12;
}
void Emulator::LD_pair_Im(int pair)
{
    uint16_t word = next_word_8t();
    switch (pair)
    {
    case 0: af(word); break;
    case 1: bc(word); break;
    case 2: de(word); break;
    case 3: hl(word); break;
    case 4: sp = word; break;
    }
    //system_cycles += 12;
}
void Emulator::LD_SP_HL()
{
    sp = hl();
    system_bonus_cycles += 4;
    //system_cycles += 8;
}
void Emulator::LDHL_SP_n()
{
    u8 _n = next_byte_4t(); // is this even being properly converted from signed to unsigned?
    int8_t n = (_n > 127) ? ((int)_n - 256) : _n; //https://stackoverflow.com/questions/7373852/interpret-unsigned-as-signed
    hl(sp + n);
    flags.cc.z = 0;
    flags.cc.n = 0;
    flags.cc.h = ((sp & 0x0F) + (n & 0x0F)) > 0x0F; // https://github.com/rtfpessoa/dmgboy/blob/master/src/Instructions.cpp
    flags.cc.c = ((sp & 0xFF) + (n & 0xFF)) > 0xFF;
    system_bonus_cycles += 4;
    //system_cycles += 12;
}
void Emulator::LD_nn_SP()
{
    uint16_t addr = next_word_8t();
    wb(addr, sp, true);
    wb(addr + 1, sp >> 8, true); 
    //system_cycles += 20;
}
void Emulator::PUSH(int pair)
{
    switch (pair)
    {
    case 0: push_word_8t(af()); break;
    case 1: push_word_8t(bc()); break;
    case 2: push_word_8t(de()); break;
    case 3: push_word_8t(hl()); break;
    }
    system_bonus_cycles += 4;
    //system_cycles += 16;
}
void Emulator::POP(int pair)
{
    switch (pair)
    {
    case 0: af(pop_word_8t()); break;
    case 1: bc(pop_word_8t()); break;
    case 2: de(pop_word_8t()); break;
    case 3: hl(pop_word_8t()); break;
    }
    //system_cycles += 12;
}

void Emulator::ADD_r(int reg, bool ADC)
{
    u8 addend = regs[reg]; 
    uint16_t full = ((uint16_t)regs[0]) + ((uint16_t)addend) + ADC * flags.cc.c;
    u8 half = (regs[0] & 0x0f) + (addend & 0x0f) + ADC * flags.cc.c;
    u8 answer = ((u8)full);
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.c = full > 0xff ? 1 : 0;
    flags.cc.h = half > 0x0f ? 1 : 0;
    regs[0] = answer;
    //system_cycles += 4;
}
void Emulator::ADD_HL(bool ADC)
{
    u8 addend = rb(hl(), true); 
    uint16_t full = ((uint16_t)regs[0]) + ((uint16_t)addend) + ADC * flags.cc.c;
    u8 half = (regs[0] & 0x0f) + (addend & 0x0f) + ADC * flags.cc.c;
    u8 answer = ((u8)full);
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.c = full > 0xff ? 1 : 0;
    flags.cc.h = half > 0x0f ? 1 : 0;
    regs[0] = answer;
    //system_cycles += 8;
}
void Emulator::ADD_im(bool ADC)
{
    u8 addend = next_byte_4t(); 
    uint16_t full = ((uint16_t)regs[0]) + ((uint16_t)addend) + ADC * flags.cc.c;
    u8 half = (regs[0] & 0x0f) + (addend & 0x0f) + ADC * flags.cc.c;
    u8 answer = ((u8)full);
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.c = full > 0xff ? 1 : 0;
    flags.cc.h = half > 0x0f ? 1 : 0;
    regs[0] = answer;
    //system_cycles += 8;
}
void Emulator::CP_HL() 
{
    u8 addend = rb(hl(), true); 
    uint16_t full = ((uint16_t)regs[0]) - ((uint16_t)addend);
    u8 answer = ((u8)full);
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 1;
    flags.cc.h = (regs[0] & 0x0f) < (addend & 0x0f);
    flags.cc.c = regs[0] < addend ? 1 : 0;
    //system_cycles += 8;
}
void Emulator::CP_r(int reg)
{
    u8 addend = regs[reg]; 
    uint16_t full = ((uint16_t)regs[0]) - ((uint16_t)addend);
    u8 answer = ((u8)full);
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 1;
    flags.cc.h = (regs[0] & 0x0f) < (addend & 0x0f);
    flags.cc.c = regs[0] < addend ? 1 : 0;
    //system_cycles += 4;
}
void Emulator::CP_im()
{
    u8 addend = next_byte_4t(); 
    uint16_t full = ((uint16_t)regs[0]) - ((uint16_t)addend);
    u8 answer = ((u8)full);
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 1;
    flags.cc.h = (regs[0] & 0x0f) < (addend & 0x0f);
    flags.cc.c = regs[0] < addend ? 1 : 0;
    //system_cycles += 8;
}
void Emulator::SUB_r(int reg, bool SBC)
{
    u8 addend = regs[reg]; 
    uint16_t full = ((uint16_t)regs[0]) - ((uint16_t)addend) - SBC * (flags.cc.c ? 1 : 0);
    u8 answer = ((u8)full);
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 1;
    flags.cc.h = (regs[0] & 0x0f) < ((addend & 0x0f) + SBC*flags.cc.c) ? 1 : 0;
    flags.cc.c = regs[0] < (addend + SBC*flags.cc.c) ? 1 : 0;
    regs[0] = answer;
    //system_cycles += 4;
}
void Emulator::SUB_HL(bool SBC)
{
    u8 addend = rb(hl(), true); 
    uint16_t full = ((uint16_t)regs[0]) - ((uint16_t)addend) - SBC * (flags.cc.c ? 1 : 0);
    u8 answer = ((u8)full);
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 1;
    flags.cc.h = (regs[0] & 0x0f) < ((addend & 0x0f) + SBC * flags.cc.c) ? 1 : 0;
    flags.cc.c = regs[0] < (addend + SBC * flags.cc.c) ? 1 : 0;
    regs[0] = answer;
    //system_cycles += 8;
}
void Emulator::SUB_im(bool SBC)
{
    u8 addend = next_byte_4t(); 
    uint16_t full = ((uint16_t)regs[0]) - ((uint16_t)addend) - SBC * (flags.cc.c ? 1 : 0);
    u8 answer = ((u8)full);
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 1;
    flags.cc.h = (regs[0] & 0x0f) < ((addend & 0x0f) + SBC * flags.cc.c) ? 1 : 0;
    flags.cc.c = regs[0] < (addend + SBC * flags.cc.c) ? 1 : 0;
    regs[0] = answer;
    //system_cycles += 8;
}
void Emulator::AND_r(int reg)
{
    u8 addend = regs[reg]; 
    u8 answer = regs[0] & addend;
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.c = 0;
    flags.cc.h = 1;
    regs[0] = answer;
    //system_cycles += 4;
}
void Emulator::AND_HL()
{
    u8 addend = rb(hl(), true); 
    u8 answer = regs[0] & addend;
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.c = 0;
    flags.cc.h = 1;
    regs[0] = answer;
    //system_cycles += 8;
}
void Emulator::AND_im()
{
    u8 addend = next_byte_4t(); 
    u8 answer = regs[0] & addend;
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.c = 0;
    flags.cc.h = 1;
    regs[0] = answer;
    //system_cycles += 8;
}
void Emulator::OR_r(int reg)
{
    u8 addend = regs[reg]; 
    u8 answer = regs[0] | addend;
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.c = 0;
    flags.cc.h = 0;
    regs[0] = answer;
    //system_cycles += 4;
}
void Emulator::OR_im()
{
    u8 addend = next_byte_4t(); 
    u8 answer = regs[0] | addend;
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.c = 0;
    flags.cc.h = 0;
    regs[0] = answer;
    //system_cycles += 8;
}
void Emulator::OR_HL()
{
    u8 addend = rb(hl(), true); 
    u8 answer = regs[0] | addend;
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.c = 0;
    flags.cc.h = 0;
    regs[0] = answer;
    //system_cycles += 8;
}
void Emulator::XOR_r(int reg)
{
    u8 addend = regs[reg]; 
    u8 answer = regs[0] ^ addend;
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.c = 0;
    flags.cc.h = 0;
    regs[0] = answer;
    //system_cycles += 4;
}
void Emulator::XOR_im()
{
    u8 addend = next_byte_4t(); 
    u8 answer = regs[0] ^ addend;
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.c = 0;
    flags.cc.h = 0;
    regs[0] = answer;
    //system_cycles += 8;
}
void Emulator::XOR_HL()
{
    u8 addend = rb(hl(), true); 
    u8 answer = regs[0] ^ addend;
    flags.cc.z = answer == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.c = 0;
    flags.cc.h = 0;
    regs[0] = answer;
    //system_cycles += 8;
}
void Emulator::INC_r(int reg)
{
    u8 value = regs[reg];
    u8 old_bit3 = value & 0x08;
    value++;
    u8 new_bit3 = value & 0x08;
    flags.cc.z = value == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.h = (old_bit3 == 0x08) && (new_bit3 == 0x00);
    regs[reg] = value;
    //system_cycles += 4;
}
void Emulator::INC_HL()
{
    u8 value = rb(hl(), true);
    u8 old_bit3 = value & 0x08;
    value++;
    u8 new_bit3 = value & 0x08;
    flags.cc.z = value == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.h = (old_bit3 == 0x08) && (new_bit3 == 0x00);
    wb(hl(), value, true);
    //system_cycles += 12;
}
void Emulator::DEC_r(int reg)
{
    u8 value = regs[reg];
    flags.cc.h = ((value & 0x0f) == 0);
    value--;
    flags.cc.z = value == 0 ? 1 : 0;
    flags.cc.n = 1;
    regs[reg] = value;
    //system_cycles += 4;
}
void Emulator::DEC_HL()
{
    u8 value = rb(hl(), true);
    flags.cc.h = ((value & 0x0f) == 0);
    value--;
    flags.cc.z = value == 0 ? 1 : 0;
    flags.cc.n = 1;
    wb(hl(), value, true);
    //system_cycles += 12;
}
void Emulator::ADD_HL(int reg)
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
    flags.cc.n = 0;
    flags.cc.h = (((hl() & 0x0FFF) + (addend & 0x0FFF)) > 0x0FFF) ? 1 : 0; // from DMGBoy
    flags.cc.c = ((hl() + addend) > 0xFFFF) ? 1 : 0; 
    hl(hl() + addend);
    system_bonus_cycles += 4;
    //system_cycles += 8;
}
void Emulator::ADD_SP()
{
    int8_t n = next_byte_4t();
    flags.cc.z = 0;
    flags.cc.n = 0;
    flags.cc.h = ((sp & 0x0F) + (n & 0x0F)) > 0x0F; // DMGBoy
    flags.cc.c = ((sp & 0xFF) + (n & 0xFF)) > 0xFF; 
    sp += n;
    system_bonus_cycles += 8;
    //system_cycles += 16;
}
void Emulator::INC_16(int reg)
{
    switch (reg)
    {
    case 1: bc(bc() + 1); break;
    case 2: de(de() + 1); break;
    case 3: hl(hl() + 1); break;
    case 4: sp++;        break;
    }
    system_bonus_cycles += 4;
    //system_cycles += 8;
}
void Emulator::DEC_16(int reg)
{
    switch (reg)
    {
    case 1: bc(bc() - 1); break;
    case 2: de(de() - 1); break;
    case 3: hl(hl() - 1); break;
    case 4: sp--;         break;
    }
    system_bonus_cycles += 4;
    //system_cycles += 8;
}

void Emulator::DAA()
{
    int _a = a();
    if (flags.cc.n == 0)
    {
        if (flags.cc.h || ((_a & 0xF) > 9)) _a += 0x06;
        if (flags.cc.c || (_a > 0x9F)) _a += 0x60;
    }
    else
    {
        if (flags.cc.h) _a = (_a - 6) & 0xFF;
        if (flags.cc.c) _a -= 0x60;
    }
    flags.cc.h = 0;
    flags.cc.z = 0;
    if ((_a & 0x100) == 0x100) flags.cc.c = 1;
    _a &= 0xFF;
    if (_a == 0) flags.cc.z = 1;
    a(_a);
    //system_cycles += 4;
}
void Emulator::CPL()
{
    a(~a());
    flags.cc.n = 1;
    flags.cc.h = 1;
    //system_cycles += 4;
}
void Emulator::CCF()
{
    flags.cc.c = ~flags.cc.c;
    flags.cc.n = 0;
    flags.cc.h = 0;
    //system_cycles += 4;
}
void Emulator::SCF()
{
    flags.cc.c = 1;
    flags.cc.n = 0;
    flags.cc.h = 0;
    //system_cycles += 4;
}
void Emulator::HALT()
{
    // The HALT instruction waits for (IF & IE) to be non-zero (IME does not matter). 
    // Blargg's test sets up a timer interrupt with IME disabled, executes HALT, then checks that IF has the timer interrupt requested.
    while (!(reg_FF0F_interrupt_flag & reg_FFFF_interrupt_en))
    {
        update_timers(4);
        update_gpu(4);
        update_interrupts();
    }
    //system_cycles += 4;
}
void Emulator::STOP()
{
    pc++;
    //system_cycles += 4;
}
void Emulator::DI()
{
    system_master_interrupt_en = false;
    //system_cycles += 4;
}
void Emulator::EI()
{
    system_master_interrupt_en = true;
    //system_cycles += 4;
}
void Emulator::RLCA()
{
    // https://hax.iimarckus.org/topic/1617/
    u8 bit7 = (a() >> 7);
    a((a() << 1) | bit7);
    flags.cc.c = bit7;
    flags.cc.z = 0;
    flags.cc.n = 0;
    flags.cc.h = 0;
    //system_cycles += 4;
}
void Emulator::RLA()
{
    u8 bit7 = (a() >> 7);
    a((a() << 1) | flags.cc.c);
    flags.cc.c = bit7;
    flags.cc.z = 0;
    flags.cc.n = 0;
    flags.cc.h = 0;
    //system_cycles += 4;
}
void Emulator::RRCA()
{
    u8 old_bit0 = a() & 0x1;
    a((a() >> 1) | (old_bit0 << 7));
    flags.cc.c = old_bit0;
    flags.cc.z = 0;
    flags.cc.n = 0;
    flags.cc.h = 0;
    //system_cycles += 4;
}
void Emulator::RRA()
{
    u8 old_bit0 = a() & 0x1;
    a((a() >> 1) | (((u8)flags.cc.c) << 7));
    flags.cc.c = old_bit0;
    flags.cc.z = 0;
    flags.cc.n = 0;
    flags.cc.h = 0;
    //system_cycles += 4;
}
void Emulator::JP(bool condition)
{
    uint16_t addr = next_word_8t();
    if (condition)
    {
        pc = addr;
        system_bonus_cycles += 4;
        //system_cycles += 16;
    }
    //else //system_cycles += 12;
}
void Emulator::JP_HL()
{
    pc = hl();
    //system_cycles += 4;
}
void Emulator::JR(bool condition)
{
    int8_t offset = next_byte_4t();
    if (condition)
    {
        pc += offset;
        system_bonus_cycles += 4;
        //system_cycles += 12;
    }
    //else //system_cycles += 8;
}
void Emulator::CALL(bool condition)
{
    uint16_t addr = next_word_8t();
    if (condition)
    {
        push_word_8t(pc);
        pc = addr;
        system_bonus_cycles += 4;
        //system_cycles += 24;
    }
    //else //system_cycles += 12;
}
void Emulator::RST(uint16_t addr)
{
    push_word_8t(pc);
    pc = addr;
    system_bonus_cycles += 4;
    //system_cycles += 16;
}
void Emulator::RET()
{
    pc = pop_word_8t();
    system_bonus_cycles += 4;
    //system_cycles += 16;
}
void Emulator::RET_cc(bool condition)
{
    if (condition)
    {
        pc = pop_word_8t();
        system_bonus_cycles += 8;
        //system_cycles += 20;
    }
    else
    {
        system_bonus_cycles += 4;
        //system_cycles += 8;
    }
}
void Emulator::RETI()
{
    pc = pop_word_8t();
    system_master_interrupt_en = true;
    system_bonus_cycles += 4;
    //system_cycles += 16;
}

void Emulator::execute_opcode(u8 opcode)
{
    switch (opcode)
    {
    case 0x46: LD_r_pair(1, 3); break;
    case 0x4e: LD_r_pair(2, 3); break;
    case 0x56: LD_r_pair(3, 3); break;
    case 0x5e: LD_r_pair(4, 3); break;
    case 0x66: LD_r_pair(5, 3); break;
    case 0x6e: LD_r_pair(6, 3); break;
    case 0x7e: LD_r_pair(0, 3); break;
    case 0x0a: LD_r_pair(0, 1); break;
    case 0x1a: LD_r_pair(0, 2); break;
    case 0x2a: LD_r_pair(0, 3, false, true); break;
    case 0x3a: LD_r_pair(0, 3, true, false); break;
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
    case 0x40: LD_r_r(1, 1); break;
    case 0x41: LD_r_r(1, 2); break;
    case 0x42: LD_r_r(1, 3); break;
    case 0x43: LD_r_r(1, 4); break;
    case 0x44: LD_r_r(1, 5); break;
    case 0x45: LD_r_r(1, 6); break;
    case 0x47: LD_r_r(1, 0); break;
    case 0x48: LD_r_r(2, 1); break;
    case 0x49: LD_r_r(2, 2); break;
    case 0x4a: LD_r_r(2, 3); break;
    case 0x4b: LD_r_r(2, 4); break;
    case 0x4c: LD_r_r(2, 5); break;
    case 0x4d: LD_r_r(2, 6); break;
    case 0x4f: LD_r_r(2, 0); break;
    case 0x50: LD_r_r(3, 1); break;
    case 0x51: LD_r_r(3, 2); break;
    case 0x52: LD_r_r(3, 3); break;
    case 0x53: LD_r_r(3, 4); break;
    case 0x54: LD_r_r(3, 5); break;
    case 0x55: LD_r_r(3, 6); break;
    case 0x57: LD_r_r(3, 0); break;
    case 0x58: LD_r_r(4, 1); break;
    case 0x59: LD_r_r(4, 2); break;
    case 0x5a: LD_r_r(4, 3); break;
    case 0x5b: LD_r_r(4, 4); break;
    case 0x5c: LD_r_r(4, 5); break;
    case 0x5d: LD_r_r(4, 6); break;
    case 0x5f: LD_r_r(4, 0); break;
    case 0x60: LD_r_r(5, 1); break;
    case 0x61: LD_r_r(5, 2); break;
    case 0x62: LD_r_r(5, 3); break;
    case 0x63: LD_r_r(5, 4); break;
    case 0x64: LD_r_r(5, 5); break;
    case 0x65: LD_r_r(5, 6); break;
    case 0x67: LD_r_r(5, 0); break;
    case 0x68: LD_r_r(6, 1); break;
    case 0x69: LD_r_r(6, 2); break;
    case 0x6a: LD_r_r(6, 3); break;
    case 0x6b: LD_r_r(6, 4); break;
    case 0x6c: LD_r_r(6, 5); break;
    case 0x6d: LD_r_r(6, 6); break;
    case 0x6f: LD_r_r(6, 0); break;
    case 0x78: LD_r_r(0, 1); break;
    case 0x79: LD_r_r(0, 2); break;
    case 0x7a: LD_r_r(0, 3); break;
    case 0x7b: LD_r_r(0, 4); break;
    case 0x7c: LD_r_r(0, 5); break;
    case 0x7d: LD_r_r(0, 6); break;
    case 0x7f: LD_r_r(0, 0); break;
    case 0x06: LD_r_Im(1); break;
    case 0x0e: LD_r_Im(2); break;
    case 0x16: LD_r_Im(3); break;
    case 0x1e: LD_r_Im(4); break;
    case 0x26: LD_r_Im(5); break;
    case 0x2e: LD_r_Im(6); break;
    case 0x3e: LD_r_Im(0); break;
    case 0x01: LD_pair_Im(1); break;
    case 0x11: LD_pair_Im(2); break;
    case 0x21: LD_pair_Im(3); break;
    case 0x31: LD_pair_Im(4); break;
    case 0x36: LD_HL_Im(); break;
    case 0x08: LD_nn_SP(); break;
    case 0xf9: LD_SP_HL(); break;
    case 0xf8: LDHL_SP_n(); break;
    case 0xde: SUB_im(true); break;
    case 0xd6: SUB_im(); break;
    case 0x98: SUB_r(1, true); break;
    case 0x99: SUB_r(2, true); break;
    case 0x9a: SUB_r(3, true); break;
    case 0x9b: SUB_r(4, true); break;
    case 0x9c: SUB_r(5, true); break;
    case 0x9d: SUB_r(6, true); break;
    case 0x9f: SUB_r(0, true); break;
    case 0x9e: SUB_HL(true); break;
    case 0x90: SUB_r(1); break;
    case 0x91: SUB_r(2); break;
    case 0x92: SUB_r(3); break;
    case 0x93: SUB_r(4); break;
    case 0x94: SUB_r(5); break;
    case 0x95: SUB_r(6); break;
    case 0x97: SUB_r(0); break;
    case 0x96: SUB_HL(); break;
    case 0xb8: CP_r(1); break;
    case 0xb9: CP_r(2); break;
    case 0xba: CP_r(3); break;
    case 0xbb: CP_r(4); break;
    case 0xbc: CP_r(5); break;
    case 0xbd: CP_r(6); break;
    case 0xbf: CP_r(0); break;
    case 0xbe: CP_HL(); break;
    case 0xfe: CP_im(); break;
    case 0xc6: ADD_im(); break;
    case 0xce: ADD_im(true); break;
    case 0x88: ADD_r(1, true); break;
    case 0x89: ADD_r(2, true); break;
    case 0x8a: ADD_r(3, true); break;
    case 0x8b: ADD_r(4, true); break;
    case 0x8c: ADD_r(5, true); break;
    case 0x8d: ADD_r(6, true); break;
    case 0x8f: ADD_r(0, true); break;
    case 0x8e: ADD_HL(true); break;
    case 0x80: ADD_r(1); break;
    case 0x81: ADD_r(2); break;
    case 0x82: ADD_r(3); break;
    case 0x83: ADD_r(4); break;
    case 0x84: ADD_r(5); break;
    case 0x85: ADD_r(6); break;
    case 0x87: ADD_r(0); break;
    case 0x86: ADD_HL(); break;
    case 0x09: ADD_HL(1); break;
    case 0x19: ADD_HL(2); break;
    case 0x29: ADD_HL(3); break;
    case 0x39: ADD_HL(4); break;
    case 0xe8: ADD_SP(); break;
    case 0xa0: AND_r(1); break;
    case 0xa1: AND_r(2); break;
    case 0xa2: AND_r(3); break;
    case 0xa3: AND_r(4); break;
    case 0xa4: AND_r(5); break;
    case 0xa5: AND_r(6); break;
    case 0xa7: AND_r(0); break;
    case 0xa6: AND_HL(); break;
    case 0xe6: AND_im(); break;
    case 0xee: XOR_im(); break;
    case 0xa8: XOR_r(1); break;
    case 0xa9: XOR_r(2); break;
    case 0xaa: XOR_r(3); break;
    case 0xab: XOR_r(4); break;
    case 0xac: XOR_r(5); break;
    case 0xad: XOR_r(6); break;
    case 0xaf: XOR_r(0); break;
    case 0xae: XOR_HL(); break;
    case 0xb0: OR_r(1); break;
    case 0xb1: OR_r(2); break;
    case 0xb2: OR_r(3); break;
    case 0xb3: OR_r(4); break;
    case 0xb4: OR_r(5); break;
    case 0xb5: OR_r(6); break;
    case 0xb7: OR_r(0); break;
    case 0xb6: OR_HL(); break;
    case 0xf6: OR_im(); break;
    case 0x3c: INC_r(0); break;
    case 0x04: INC_r(1); break;
    case 0x0c: INC_r(2); break;
    case 0x14: INC_r(3); break;
    case 0x1c: INC_r(4); break;
    case 0x24: INC_r(5); break;
    case 0x2c: INC_r(6); break;
    case 0x34: INC_HL(); break;
    case 0x05: DEC_r(1); break;
    case 0x0d: DEC_r(2); break;
    case 0x15: DEC_r(3); break;
    case 0x25: DEC_r(5); break;
    case 0x2d: DEC_r(6); break;
    case 0x3d: DEC_r(0); break;
    case 0x1d: DEC_r(4); break;
    case 0x35: DEC_HL(); break;
    case 0x03: INC_16(1); break;
    case 0x13: INC_16(2); break;
    case 0x23: INC_16(3); break;
    case 0x33: INC_16(4); break;
    case 0x0b: DEC_16(1); break;
    case 0x1b: DEC_16(2); break;
    case 0x2b: DEC_16(3); break;
    case 0x3b: DEC_16(4); break;
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
    case 0xc9: RET(); break;
    case 0xc0: RET_cc(flags.cc.z == 0); break;
    case 0xc8: RET_cc(flags.cc.z == 1); break;
    case 0xd0: RET_cc(flags.cc.c == 0); break;
    case 0xd8: RET_cc(flags.cc.c == 1); break;
    case 0xe9: JP_HL(); break;
    case 0x27: DAA(); break;
    case 0x2f: CPL(); break;
    case 0x37: SCF(); break;
    case 0x3f: CCF(); break;
    case 0x17: RLA(); break;
    case 0x1f: RRA(); break;
    case 0x10: STOP(); break;
    case 0x07: RLCA(); break;
    case 0x0f: RRCA(); break;
    case 0xd9: RETI(); break;
    case 0xf3: DI(); break;
    case 0xfb: EI(); break;
    case 0x76: HALT(); break;
    case 0xc5: PUSH(1); break;
    case 0xd5: PUSH(2); break;
    case 0xe5: PUSH(3); break;
    case 0xf5: PUSH(0); break;
    case 0xc1: POP(1); break;
    case 0xd1: POP(2); break;
    case 0xe1: POP(3); break;
    case 0xf1: POP(0); break;
    case 0xc7: RST(0x00); break;
    case 0xcf: RST(0x08); break;
    case 0xd7: RST(0x10); break;
    case 0xdf: RST(0x18); break;
    case 0xe7: RST(0x20); break;
    case 0xef: RST(0x28); break;
    case 0xf7: RST(0x30); break;
    case 0xff: RST(0x38); break;
    case 0x00: break; //system_cycles += 4; break; // NOP
    case 0xcb: 
    {
        u8 opcode = next_byte_4t();
        execute_extended_opcode(opcode); break;
    }
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
}
void Emulator::cb_SWAP(int reg, bool HL)
{
    u8 value, answer;
    if (HL) value = rb(hl(), true);
    else value = regs[reg];

    u8 lower_nib = value & 0x0f;
    u8 upper_nib = value & 0xf0;
    answer = (upper_nib >> 4) | (lower_nib << 4);
    flags.cc.z = answer == 0;
    flags.cc.c = flags.cc.n = flags.cc.h = 0;

    if (HL)
    {
        wb(hl(), answer, true);
        //system_cycles += 16;
    }
    else
    {
        regs[reg] = answer;
        //system_cycles += 8;
    }
}
void Emulator::cb_RLC(int reg, bool HL)
{
    u8 value, answer;
    if (HL) value = rb(hl(), true);
    else value = regs[reg];

    u8 old_bit7 = (value >> 7) & 0x1;
    answer = (value << 1) | old_bit7;
    flags.cc.c = old_bit7;
    flags.cc.n = 0;
    flags.cc.h = 0;
    flags.cc.z = answer == 0 ? 1 : 0;

    if (HL)
    {
        wb(hl(), answer, true);
        //system_cycles += 16;
    }
    else
    {
        regs[reg] = answer;
        //system_cycles += 8;
    }
}
void Emulator::cb_RL(int reg, bool HL)
{
    u8 value, answer;
    if (HL) value = rb(hl(), true);
    else value = regs[reg];

    u8 old_bit7 = (value >> 7);
    answer = (value << 1) | flags.cc.c;
    flags.cc.c = old_bit7;
    flags.cc.n = 0;
    flags.cc.h = 0;
    flags.cc.z = answer == 0 ? 1 : 0;

    if (HL)
    {
        wb(hl(), answer, true);
        //system_cycles += 16;
    }
    else
    {
        regs[reg] = answer;
        //system_cycles += 8;
    }
}
void Emulator::cb_RRC(int reg, bool HL)
{
    u8 value, answer;
    if (HL) value = rb(hl(), true);
    else value = regs[reg];

    u8 old_bit0 = value & 0x1;
    answer = (value >> 1) | (old_bit0 << 7);
    flags.cc.c = old_bit0;
    flags.cc.n = 0;
    flags.cc.h = 0;
    flags.cc.z = answer == 0 ? 1 : 0;

    if (HL)
    {
        wb(hl(), answer, true);
        //system_cycles += 16;
    }
    else
    {
        regs[reg] = answer;
        //system_cycles += 8;
    }
}
void Emulator::cb_RR(int reg, bool HL)
{
    u8 value, answer;
    if (HL) value = rb(hl(), true);
    else value = regs[reg];

    u8 old_bit0 = value & 0x1;
    answer = (value >> 1) | (((u8)flags.cc.c) << 7);
    flags.cc.c = old_bit0;
    flags.cc.n = 0;
    flags.cc.h = 0;
    flags.cc.z = answer == 0 ? 1 : 0;

    if (HL)
    {
        wb(hl(), answer, true);
        //system_cycles += 16;
    }
    else
    {
        regs[reg] = answer;
        //system_cycles += 8;
    }
}
void Emulator::cb_SLA(int reg, bool HL)
{
    u8 value, answer;
    if (HL) value = rb(hl(), true);
    else value = regs[reg];

    u8 old_bit7 = (value >> 7);
    answer = value << 1;
    flags.cc.c = old_bit7;
    flags.cc.n = 0;
    flags.cc.h = 0;
    flags.cc.z = answer == 0 ? 1 : 0;

    if (HL)
    {
        wb(hl(), answer, true);
        //system_cycles += 16;
    }
    else
    {
        regs[reg] = answer;
        //system_cycles += 8;
    }
}
void Emulator::cb_SRA(int reg, bool HL)
{
    u8 value, answer;
    if (HL) value = rb(hl(), true);
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
        wb(hl(), answer, true);
        //system_cycles += 16;
    }
    else
    {
        regs[reg] = answer;
        //system_cycles += 8;
    }
}
void Emulator::cb_SRL(int reg, bool HL)
{
    u8 value, answer;
    if (HL) value = rb(hl(), true);
    else value = regs[reg];

    u8 old_bit0 = value & 0x1;
    answer = value >> 1;
    flags.cc.c = old_bit0;
    flags.cc.n = 0;
    flags.cc.h = 0;
    flags.cc.z = answer == 0 ? 1 : 0;

    if (HL)
    {
        wb(hl(), answer, true);
        //system_cycles += 16;
    }
    else
    {
        regs[reg] = answer;
        //system_cycles += 8;
    }
}
void Emulator::cb_BIT(int bit, int reg, bool HL)
{
    u8 value;
    if (HL)
    {
        //system_cycles += 12;
        value = rb(hl(), true);
    }
    else
    {
        //system_cycles += 8;
        value = regs[reg];
    }

    value = (value >> bit) & 0x1;

    flags.cc.z = value == 0 ? 1 : 0;
    flags.cc.n = 0;
    flags.cc.h = 1;
}
void Emulator::cb_RESET(int bit, int reg, bool HL)
{
    u8 value, answer;
    if (HL) value = rb(hl(), true);
    else value = regs[reg];

    answer = value & ~(0x1 << bit);

    if (HL)
    {
        wb(hl(), answer, true);
        //system_cycles += 16;
    }
    else
    {
        regs[reg] = answer;
        //system_cycles += 8;
    }
}
void Emulator::cb_SET(int bit, int reg, bool HL)
{
    u8 value, answer;
    if (HL) value = rb(hl(), true);
    else value = regs[reg];

    answer = value | (0x1 << bit);

    if (HL)
    {
        wb(hl(), answer, true);
        //system_cycles += 16;
    }
    else
    {
        regs[reg] = answer;
        //system_cycles += 8;
    }
}
void Emulator::execute_extended_opcode(u8 opcode)
{
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