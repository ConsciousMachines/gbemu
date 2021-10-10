#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <stdio.h>

typedef uint8_t u8;
typedef uint16_t u16;

struct dis_line
{
	u16 _address;
	int _size;
	u8 _bytes[3];
	char _text[20];
};

dis_line disassemble(u8* bytes, u16 pc)
{
	u8 b1 = bytes[pc + 0];
	u8 b2 = bytes[pc + 1];
	u8 b3 = bytes[pc + 2];

	switch (b1)
	{
	case 0x00: return dis_line{ pc, 1, {b1,b2,b3},"NOP\0" };
	case 0x01: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "LD BC,$%02x%02x\0", b3, b2); return l; }
	case 0x02: return dis_line{ pc, 1, {b1,b2,b3},"LD (BC),A\0" };
	case 0x03: return dis_line{ pc, 1, {b1,b2,b3},"INC BC\0" };
	case 0x04: return dis_line{ pc, 1, {b1,b2,b3},"INC B\0" };
	case 0x05: return dis_line{ pc, 1, {b1,b2,b3},"DEC B\0" };
	case 0x06: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "LD B,$%02x\0", b2); return l; }
	case 0x07: return dis_line{ pc, 1, {b1,b2,b3},"RLCA\0" };
	case 0x08: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "LD ($%02x%02x),SP\0", b3, b2); return l; }
	case 0x09: return dis_line{ pc, 1, {b1,b2,b3},"ADD HL,BC\0" };
	case 0x0A: return dis_line{ pc, 1, {b1,b2,b3},"LD A,(BC)\0" };
	case 0x0B: return dis_line{ pc, 1, {b1,b2,b3},"DEC BC\0" };
	case 0x0C: return dis_line{ pc, 1, {b1,b2,b3},"INC C\0" };
	case 0x0D: return dis_line{ pc, 1, {b1,b2,b3},"DEC C\0" };
	case 0x0E: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "LD C,$%02x\0", b2); return l; }
	case 0x0F: return dis_line{ pc, 1, {b1,b2,b3},"RRCA\0" };
	case 0x10: return dis_line{ pc, 2, {b1,b2,b3},"STOP\0" };
	case 0x11: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "LD DE,$%02x%02x\0", b3, b2); return l; }
	case 0x12: return dis_line{ pc, 1, {b1,b2,b3},"LD (DE),A\0" };
	case 0x13: return dis_line{ pc, 1, {b1,b2,b3},"INC DE\0" };
	case 0x14: return dis_line{ pc, 1, {b1,b2,b3},"INC D\0" };
	case 0x15: return dis_line{ pc, 1, {b1,b2,b3},"DEC D\0" };
	case 0x16: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "LD D,$%02x\0", b2); return l; }
	case 0x17: return dis_line{ pc, 1, {b1,b2,b3},"RLA\0" };
	case 0x18: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "JR $%02x [%hhi]\0", b2, b2); return l; }
	case 0x19: return dis_line{ pc, 1, {b1,b2,b3},"ADD HL,DE\0" };
	case 0x1A: return dis_line{ pc, 1, {b1,b2,b3},"LD A,(DE)\0" };
	case 0x1B: return dis_line{ pc, 1, {b1,b2,b3},"DEC DE\0" };
	case 0x1C: return dis_line{ pc, 1, {b1,b2,b3},"INC E\0" };
	case 0x1D: return dis_line{ pc, 1, {b1,b2,b3},"DEC E\0" };
	case 0x1E: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "LD E,$%02x\0", b2); return l; }
	case 0x1F: return dis_line{ pc, 1, {b1,b2,b3},"RRA\0" };
	case 0x20: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "JR NZ,$%02x [%hhi]\0", b2, b2); return l; }
	case 0x21: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "LD HL,$%02x%02x\0", b3, b2); return l; }
	case 0x22: return dis_line{ pc, 1, {b1,b2,b3},"LD (HL+),A\0" };
	case 0x23: return dis_line{ pc, 1, {b1,b2,b3},"INC HL\0" };
	case 0x24: return dis_line{ pc, 1, {b1,b2,b3},"INC H\0" };
	case 0x25: return dis_line{ pc, 1, {b1,b2,b3},"DEC H\0" };
	case 0x26: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "LD H,$%02x\0", b2); return l; }
	case 0x27: return dis_line{ pc, 1, {b1,b2,b3},"DAA\0" };
	case 0x28: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "JR Z,$%02x [%hhi]\0", b2, b2); return l; }
	case 0x29: return dis_line{ pc, 1, {b1,b2,b3},"ADD HL,HL\0" };
	case 0x2A: return dis_line{ pc, 1, {b1,b2,b3},"LD A,(HL+)\0" };
	case 0x2B: return dis_line{ pc, 1, {b1,b2,b3},"DEC HL\0" };
	case 0x2C: return dis_line{ pc, 1, {b1,b2,b3},"INC L\0" };
	case 0x2D: return dis_line{ pc, 1, {b1,b2,b3},"DEC L\0" };
	case 0x2E: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "LD L,$%02x\0", b2); return l; }
	case 0x2F: return dis_line{ pc, 1, {b1,b2,b3},"CPL\0" };
	case 0x30: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "JR NC,$%02x [%hhi]\0", b2, b2); return l; }
	case 0x31: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "LD SP,$%02x%02x\0", b3, b2); return l; }
	case 0x32: return dis_line{ pc, 1, {b1,b2,b3},"LD (HL-),A\0" };
	case 0x33: return dis_line{ pc, 1, {b1,b2,b3},"INC SP\0" };
	case 0x34: return dis_line{ pc, 1, {b1,b2,b3},"INC (HL)\0" };
	case 0x35: return dis_line{ pc, 1, {b1,b2,b3},"DEC (HL)\0" };
	case 0x36: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "LD (HL),$%02x\0", b2); return l; }
	case 0x37: return dis_line{ pc, 1, {b1,b2,b3},"SCF\0" };
	case 0x38: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "JR C,$%02x [%hhi]\0", b2, b2); return l; }
	case 0x39: return dis_line{ pc, 1, {b1,b2,b3},"ADD HL,SP\0" };
	case 0x3A: return dis_line{ pc, 1, {b1,b2,b3},"LD A,(HL-)\0" };
	case 0x3B: return dis_line{ pc, 1, {b1,b2,b3},"DEC SP\0" };
	case 0x3C: return dis_line{ pc, 1, {b1,b2,b3},"INC A\0" };
	case 0x3D: return dis_line{ pc, 1, {b1,b2,b3},"DEC A\0" };
	case 0x3E: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "LD A,$%02x\0", b2); return l; }
	case 0x3F: return dis_line{ pc, 1, {b1,b2,b3},"CCF\0" };
	case 0x40: return dis_line{ pc, 1, {b1,b2,b3},"LD B,B\0" };
	case 0x41: return dis_line{ pc, 1, {b1,b2,b3},"LD B,C\0" };
	case 0x42: return dis_line{ pc, 1, {b1,b2,b3},"LD B,D\0" };
	case 0x43: return dis_line{ pc, 1, {b1,b2,b3},"LD B,E\0" };
	case 0x44: return dis_line{ pc, 1, {b1,b2,b3},"LD B,H\0" };
	case 0x45: return dis_line{ pc, 1, {b1,b2,b3},"LD B,L\0" };
	case 0x46: return dis_line{ pc, 1, {b1,b2,b3},"LD B,(HL)\0" };
	case 0x47: return dis_line{ pc, 1, {b1,b2,b3},"LD B,A\0" };
	case 0x48: return dis_line{ pc, 1, {b1,b2,b3},"LD C,B\0" };
	case 0x49: return dis_line{ pc, 1, {b1,b2,b3},"LD C,C\0" };
	case 0x4A: return dis_line{ pc, 1, {b1,b2,b3},"LD C,D\0" };
	case 0x4B: return dis_line{ pc, 1, {b1,b2,b3},"LD C,E\0" };
	case 0x4C: return dis_line{ pc, 1, {b1,b2,b3},"LD C,H\0" };
	case 0x4D: return dis_line{ pc, 1, {b1,b2,b3},"LD C,L\0" };
	case 0x4E: return dis_line{ pc, 1, {b1,b2,b3},"LD C,(HL)\0" };
	case 0x4F: return dis_line{ pc, 1, {b1,b2,b3},"LD C,A\0" };
	case 0x50: return dis_line{ pc, 1, {b1,b2,b3},"LD D,B\0" };
	case 0x51: return dis_line{ pc, 1, {b1,b2,b3},"LD D,C\0" };
	case 0x52: return dis_line{ pc, 1, {b1,b2,b3},"LD D,D\0" };
	case 0x53: return dis_line{ pc, 1, {b1,b2,b3},"LD D,E\0" };
	case 0x54: return dis_line{ pc, 1, {b1,b2,b3},"LD D,H\0" };
	case 0x55: return dis_line{ pc, 1, {b1,b2,b3},"LD D,L\0" };
	case 0x56: return dis_line{ pc, 1, {b1,b2,b3},"LD D,(HL)\0" };
	case 0x57: return dis_line{ pc, 1, {b1,b2,b3},"LD D,A\0" };
	case 0x58: return dis_line{ pc, 1, {b1,b2,b3},"LD E,B\0" };
	case 0x59: return dis_line{ pc, 1, {b1,b2,b3},"LD E,C\0" };
	case 0x5A: return dis_line{ pc, 1, {b1,b2,b3},"LD E,D\0" };
	case 0x5B: return dis_line{ pc, 1, {b1,b2,b3},"LD E,E\0" };
	case 0x5C: return dis_line{ pc, 1, {b1,b2,b3},"LD E,H\0" };
	case 0x5D: return dis_line{ pc, 1, {b1,b2,b3},"LD E,L\0" };
	case 0x5E: return dis_line{ pc, 1, {b1,b2,b3},"LD E,(HL)\0" };
	case 0x5F: return dis_line{ pc, 1, {b1,b2,b3},"LD E,A\0" };
	case 0x60: return dis_line{ pc, 1, {b1,b2,b3},"LD H,B\0" };
	case 0x61: return dis_line{ pc, 1, {b1,b2,b3},"LD H,C\0" };
	case 0x62: return dis_line{ pc, 1, {b1,b2,b3},"LD H,D\0" };
	case 0x63: return dis_line{ pc, 1, {b1,b2,b3},"LD H,E\0" };
	case 0x64: return dis_line{ pc, 1, {b1,b2,b3},"LD H,H\0" };
	case 0x65: return dis_line{ pc, 1, {b1,b2,b3},"LD H,L\0" };
	case 0x66: return dis_line{ pc, 1, {b1,b2,b3},"LD H,(HL)\0" };
	case 0x67: return dis_line{ pc, 1, {b1,b2,b3},"LD H,A\0" };
	case 0x68: return dis_line{ pc, 1, {b1,b2,b3},"LD L,B\0" };
	case 0x69: return dis_line{ pc, 1, {b1,b2,b3},"LD L,C\0" };
	case 0x6A: return dis_line{ pc, 1, {b1,b2,b3},"LD L,D\0" };
	case 0x6B: return dis_line{ pc, 1, {b1,b2,b3},"LD L,E\0" };
	case 0x6C: return dis_line{ pc, 1, {b1,b2,b3},"LD L,H\0" };
	case 0x6D: return dis_line{ pc, 1, {b1,b2,b3},"LD L,L\0" };
	case 0x6E: return dis_line{ pc, 1, {b1,b2,b3},"LD L,(HL)\0" };
	case 0x6F: return dis_line{ pc, 1, {b1,b2,b3},"LD L,A\0" };
	case 0x70: return dis_line{ pc, 1, {b1,b2,b3},"LD (HL),B\0" };
	case 0x71: return dis_line{ pc, 1, {b1,b2,b3},"LD (HL),C\0" };
	case 0x72: return dis_line{ pc, 1, {b1,b2,b3},"LD (HL),D\0" };
	case 0x73: return dis_line{ pc, 1, {b1,b2,b3},"LD (HL),E\0" };
	case 0x74: return dis_line{ pc, 1, {b1,b2,b3},"LD (HL),H\0" };
	case 0x75: return dis_line{ pc, 1, {b1,b2,b3},"LD (HL),L\0" };
	case 0x76: return dis_line{ pc, 1, {b1,b2,b3},"HALT\0" };
	case 0x77: return dis_line{ pc, 1, {b1,b2,b3},"LD (HL),A\0" };
	case 0x78: return dis_line{ pc, 1, {b1,b2,b3},"LD A,B\0" };
	case 0x79: return dis_line{ pc, 1, {b1,b2,b3},"LD A,C\0" };
	case 0x7A: return dis_line{ pc, 1, {b1,b2,b3},"LD A,D\0" };
	case 0x7B: return dis_line{ pc, 1, {b1,b2,b3},"LD A,E\0" };
	case 0x7C: return dis_line{ pc, 1, {b1,b2,b3},"LD A,H\0" };
	case 0x7D: return dis_line{ pc, 1, {b1,b2,b3},"LD A,L\0" };
	case 0x7E: return dis_line{ pc, 1, {b1,b2,b3},"LD A,(HL)\0" };
	case 0x7F: return dis_line{ pc, 1, {b1,b2,b3},"LD A,A\0" };
	case 0x80: return dis_line{ pc, 1, {b1,b2,b3},"ADD A,B\0" };
	case 0x81: return dis_line{ pc, 1, {b1,b2,b3},"ADD A,C\0" };
	case 0x82: return dis_line{ pc, 1, {b1,b2,b3},"ADD A,D\0" };
	case 0x83: return dis_line{ pc, 1, {b1,b2,b3},"ADD A,E\0" };
	case 0x84: return dis_line{ pc, 1, {b1,b2,b3},"ADD A,H\0" };
	case 0x85: return dis_line{ pc, 1, {b1,b2,b3},"ADD A,L\0" };
	case 0x86: return dis_line{ pc, 1, {b1,b2,b3},"ADD A,(HL)\0" };
	case 0x87: return dis_line{ pc, 1, {b1,b2,b3},"ADD A,A\0" };
	case 0x88: return dis_line{ pc, 1, {b1,b2,b3},"ADC A,B\0" };
	case 0x89: return dis_line{ pc, 1, {b1,b2,b3},"ADC A,C\0" };
	case 0x8A: return dis_line{ pc, 1, {b1,b2,b3},"ADC A,D\0" };
	case 0x8B: return dis_line{ pc, 1, {b1,b2,b3},"ADC A,E\0" };
	case 0x8C: return dis_line{ pc, 1, {b1,b2,b3},"ADC A,H\0" };
	case 0x8D: return dis_line{ pc, 1, {b1,b2,b3},"ADC A,L\0" };
	case 0x8E: return dis_line{ pc, 1, {b1,b2,b3},"ADC A,(HL)\0" };
	case 0x8F: return dis_line{ pc, 1, {b1,b2,b3},"ADC A,A\0" };
	case 0x90: return dis_line{ pc, 1, {b1,b2,b3},"SUB A,B\0" };
	case 0x91: return dis_line{ pc, 1, {b1,b2,b3},"SUB A,C\0" };
	case 0x92: return dis_line{ pc, 1, {b1,b2,b3},"SUB A,D\0" };
	case 0x93: return dis_line{ pc, 1, {b1,b2,b3},"SUB A,E\0" };
	case 0x94: return dis_line{ pc, 1, {b1,b2,b3},"SUB A,H\0" };
	case 0x95: return dis_line{ pc, 1, {b1,b2,b3},"SUB A,L\0" };
	case 0x96: return dis_line{ pc, 1, {b1,b2,b3},"SUB A,(HL)\0" };
	case 0x97: return dis_line{ pc, 1, {b1,b2,b3},"SUB A,A\0" };
	case 0x98: return dis_line{ pc, 1, {b1,b2,b3},"SBC A,B\0" };
	case 0x99: return dis_line{ pc, 1, {b1,b2,b3},"SBC A,C\0" };
	case 0x9A: return dis_line{ pc, 1, {b1,b2,b3},"SBC A,D\0" };
	case 0x9B: return dis_line{ pc, 1, {b1,b2,b3},"SBC A,E\0" };
	case 0x9C: return dis_line{ pc, 1, {b1,b2,b3},"SBC A,H\0" };
	case 0x9D: return dis_line{ pc, 1, {b1,b2,b3},"SBC A,L\0" };
	case 0x9E: return dis_line{ pc, 1, {b1,b2,b3},"SBC A,(HL)\0" };
	case 0x9F: return dis_line{ pc, 1, {b1,b2,b3},"SBC A,A\0" };
	case 0xA0: return dis_line{ pc, 1, {b1,b2,b3},"AND A,B\0" };
	case 0xA1: return dis_line{ pc, 1, {b1,b2,b3},"AND A,C\0" };
	case 0xA2: return dis_line{ pc, 1, {b1,b2,b3},"AND A,D\0" };
	case 0xA3: return dis_line{ pc, 1, {b1,b2,b3},"AND A,E\0" };
	case 0xA4: return dis_line{ pc, 1, {b1,b2,b3},"AND A,H\0" };
	case 0xA5: return dis_line{ pc, 1, {b1,b2,b3},"AND A,L\0" };
	case 0xA6: return dis_line{ pc, 1, {b1,b2,b3},"AND A,(HL)\0" };
	case 0xA7: return dis_line{ pc, 1, {b1,b2,b3},"AND A,A\0" };
	case 0xA8: return dis_line{ pc, 1, {b1,b2,b3},"XOR A,B\0" };
	case 0xA9: return dis_line{ pc, 1, {b1,b2,b3},"XOR A,C\0" };
	case 0xAA: return dis_line{ pc, 1, {b1,b2,b3},"XOR A,D\0" };
	case 0xAB: return dis_line{ pc, 1, {b1,b2,b3},"XOR A,E\0" };
	case 0xAC: return dis_line{ pc, 1, {b1,b2,b3},"XOR A,H\0" };
	case 0xAD: return dis_line{ pc, 1, {b1,b2,b3},"XOR A,L\0" };
	case 0xAE: return dis_line{ pc, 1, {b1,b2,b3},"XOR A,(HL)\0" };
	case 0xAF: return dis_line{ pc, 1, {b1,b2,b3},"XOR A,A\0" };
	case 0xB0: return dis_line{ pc, 1, {b1,b2,b3},"OR A,B\0" };
	case 0xB1: return dis_line{ pc, 1, {b1,b2,b3},"OR A,C\0" };
	case 0xB2: return dis_line{ pc, 1, {b1,b2,b3},"OR A,D\0" };
	case 0xB3: return dis_line{ pc, 1, {b1,b2,b3},"OR A,E\0" };
	case 0xB4: return dis_line{ pc, 1, {b1,b2,b3},"OR A,H\0" };
	case 0xB5: return dis_line{ pc, 1, {b1,b2,b3},"OR A,L\0" };
	case 0xB6: return dis_line{ pc, 1, {b1,b2,b3},"OR A,(HL)\0" };
	case 0xB7: return dis_line{ pc, 1, {b1,b2,b3},"OR A,A\0" };
	case 0xB8: return dis_line{ pc, 1, {b1,b2,b3},"CP A,B\0" };
	case 0xB9: return dis_line{ pc, 1, {b1,b2,b3},"CP A,C\0" };
	case 0xBA: return dis_line{ pc, 1, {b1,b2,b3},"CP A,D\0" };
	case 0xBB: return dis_line{ pc, 1, {b1,b2,b3},"CP A,E\0" };
	case 0xBC: return dis_line{ pc, 1, {b1,b2,b3},"CP A,H\0" };
	case 0xBD: return dis_line{ pc, 1, {b1,b2,b3},"CP A,L\0" };
	case 0xBE: return dis_line{ pc, 1, {b1,b2,b3},"CP A,(HL)\0" };
	case 0xBF: return dis_line{ pc, 1, {b1,b2,b3},"CP A,A\0" };
	case 0xC0: return dis_line{ pc, 1, {b1,b2,b3},"RET NZ\0" };
	case 0xC1: return dis_line{ pc, 1, {b1,b2,b3},"POP BC\0" };
	case 0xC2: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "JP NZ,$%02x%02x\0", b3, b2); return l; }
	case 0xC3: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "JP $%02x%02x\0", b3, b2); return l; }
	case 0xC4: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "CALL NZ,$%02x%02x\0", b3, b2); return l; }
	case 0xC5: return dis_line{ pc, 1, {b1,b2,b3},"PUSH BC\0" };
	case 0xC6: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "ADD A,$%02x\0", b2); return l; }
	case 0xC7: return dis_line{ pc, 1, {b1,b2,b3},"RST 00h\0" };
	case 0xC8: return dis_line{ pc, 1, {b1,b2,b3},"RET Z\0" };
	case 0xC9: return dis_line{ pc, 1, {b1,b2,b3},"RET\0" };
	case 0xCA: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "JP Z,$%02x%02x\0", b3, b2); return l; }
	case 0xCB:
		switch (b2)
		{
		case 0x00: return dis_line{ pc, 2, {0xCB,0x00,0x00},"RLC B\0" };
		case 0x01: return dis_line{ pc, 2, {0xCB,0x01,0x00},"RLC C\0" };
		case 0x02: return dis_line{ pc, 2, {0xCB,0x02,0x00},"RLC D\0" };
		case 0x03: return dis_line{ pc, 2, {0xCB,0x03,0x00},"RLC E\0" };
		case 0x04: return dis_line{ pc, 2, {0xCB,0x04,0x00},"RLC H\0" };
		case 0x05: return dis_line{ pc, 2, {0xCB,0x05,0x00},"RLC L\0" };
		case 0x06: return dis_line{ pc, 2, {0xCB,0x06,0x00},"RLC (HL)\0" };
		case 0x07: return dis_line{ pc, 2, {0xCB,0x07,0x00},"RLC A\0" };
		case 0x08: return dis_line{ pc, 2, {0xCB,0x08,0x00},"RRC B\0" };
		case 0x09: return dis_line{ pc, 2, {0xCB,0x09,0x00},"RRC C\0" };
		case 0x0A: return dis_line{ pc, 2, {0xCB,0x0A,0x00},"RRC D\0" };
		case 0x0B: return dis_line{ pc, 2, {0xCB,0x0B,0x00},"RRC E\0" };
		case 0x0C: return dis_line{ pc, 2, {0xCB,0x0C,0x00},"RRC H\0" };
		case 0x0D: return dis_line{ pc, 2, {0xCB,0x0D,0x00},"RRC L\0" };
		case 0x0E: return dis_line{ pc, 2, {0xCB,0x0E,0x00},"RRC (HL)\0" };
		case 0x0F: return dis_line{ pc, 2, {0xCB,0x0F,0x00},"RRC A\0" };
		case 0x10: return dis_line{ pc, 2, {0xCB,0x10,0x00},"RL B\0" };
		case 0x11: return dis_line{ pc, 2, {0xCB,0x11,0x00},"RL C\0" };
		case 0x12: return dis_line{ pc, 2, {0xCB,0x12,0x00},"RL D\0" };
		case 0x13: return dis_line{ pc, 2, {0xCB,0x13,0x00},"RL E\0" };
		case 0x14: return dis_line{ pc, 2, {0xCB,0x14,0x00},"RL H\0" };
		case 0x15: return dis_line{ pc, 2, {0xCB,0x15,0x00},"RL L\0" };
		case 0x16: return dis_line{ pc, 2, {0xCB,0x16,0x00},"RL (HL)\0" };
		case 0x17: return dis_line{ pc, 2, {0xCB,0x17,0x00},"RL A\0" };
		case 0x18: return dis_line{ pc, 2, {0xCB,0x18,0x00},"RR B\0" };
		case 0x19: return dis_line{ pc, 2, {0xCB,0x19,0x00},"RR C\0" };
		case 0x1A: return dis_line{ pc, 2, {0xCB,0x1A,0x00},"RR D\0" };
		case 0x1B: return dis_line{ pc, 2, {0xCB,0x1B,0x00},"RR E\0" };
		case 0x1C: return dis_line{ pc, 2, {0xCB,0x1C,0x00},"RR H\0" };
		case 0x1D: return dis_line{ pc, 2, {0xCB,0x1D,0x00},"RR L\0" };
		case 0x1E: return dis_line{ pc, 2, {0xCB,0x1E,0x00},"RR (HL)\0" };
		case 0x1F: return dis_line{ pc, 2, {0xCB,0x1F,0x00},"RR A\0" };
		case 0x20: return dis_line{ pc, 2, {0xCB,0x20,0x00},"SLA B\0" };
		case 0x21: return dis_line{ pc, 2, {0xCB,0x21,0x00},"SLA C\0" };
		case 0x22: return dis_line{ pc, 2, {0xCB,0x22,0x00},"SLA D\0" };
		case 0x23: return dis_line{ pc, 2, {0xCB,0x23,0x00},"SLA E\0" };
		case 0x24: return dis_line{ pc, 2, {0xCB,0x24,0x00},"SLA H\0" };
		case 0x25: return dis_line{ pc, 2, {0xCB,0x25,0x00},"SLA L\0" };
		case 0x26: return dis_line{ pc, 2, {0xCB,0x26,0x00},"SLA (HL)\0" };
		case 0x27: return dis_line{ pc, 2, {0xCB,0x27,0x00},"SLA A\0" };
		case 0x28: return dis_line{ pc, 2, {0xCB,0x28,0x00},"SRA B\0" };
		case 0x29: return dis_line{ pc, 2, {0xCB,0x29,0x00},"SRA C\0" };
		case 0x2A: return dis_line{ pc, 2, {0xCB,0x2A,0x00},"SRA D\0" };
		case 0x2B: return dis_line{ pc, 2, {0xCB,0x2B,0x00},"SRA E\0" };
		case 0x2C: return dis_line{ pc, 2, {0xCB,0x2C,0x00},"SRA H\0" };
		case 0x2D: return dis_line{ pc, 2, {0xCB,0x2D,0x00},"SRA L\0" };
		case 0x2E: return dis_line{ pc, 2, {0xCB,0x2E,0x00},"SRA (HL)\0" };
		case 0x2F: return dis_line{ pc, 2, {0xCB,0x2F,0x00},"SRA A\0" };
		case 0x30: return dis_line{ pc, 2, {0xCB,0x30,0x00},"SWAP B\0" };
		case 0x31: return dis_line{ pc, 2, {0xCB,0x31,0x00},"SWAP C\0" };
		case 0x32: return dis_line{ pc, 2, {0xCB,0x32,0x00},"SWAP D\0" };
		case 0x33: return dis_line{ pc, 2, {0xCB,0x33,0x00},"SWAP E\0" };
		case 0x34: return dis_line{ pc, 2, {0xCB,0x34,0x00},"SWAP H\0" };
		case 0x35: return dis_line{ pc, 2, {0xCB,0x35,0x00},"SWAP L\0" };
		case 0x36: return dis_line{ pc, 2, {0xCB,0x36,0x00},"SWAP (HL)\0" };
		case 0x37: return dis_line{ pc, 2, {0xCB,0x37,0x00},"SWAP A\0" };
		case 0x38: return dis_line{ pc, 2, {0xCB,0x38,0x00},"SRL B\0" };
		case 0x39: return dis_line{ pc, 2, {0xCB,0x39,0x00},"SRL C\0" };
		case 0x3A: return dis_line{ pc, 2, {0xCB,0x3A,0x00},"SRL D\0" };
		case 0x3B: return dis_line{ pc, 2, {0xCB,0x3B,0x00},"SRL E\0" };
		case 0x3C: return dis_line{ pc, 2, {0xCB,0x3C,0x00},"SRL H\0" };
		case 0x3D: return dis_line{ pc, 2, {0xCB,0x3D,0x00},"SRL L\0" };
		case 0x3E: return dis_line{ pc, 2, {0xCB,0x3E,0x00},"SRL (HL)\0" };
		case 0x3F: return dis_line{ pc, 2, {0xCB,0x3F,0x00},"SRL A\0" };
		case 0x40: return dis_line{ pc, 2, {0xCB,0x40,0x00},"BIT 0,B\0" };
		case 0x41: return dis_line{ pc, 2, {0xCB,0x41,0x00},"BIT 0,C\0" };
		case 0x42: return dis_line{ pc, 2, {0xCB,0x42,0x00},"BIT 0,D\0" };
		case 0x43: return dis_line{ pc, 2, {0xCB,0x43,0x00},"BIT 0,E\0" };
		case 0x44: return dis_line{ pc, 2, {0xCB,0x44,0x00},"BIT 0,H\0" };
		case 0x45: return dis_line{ pc, 2, {0xCB,0x45,0x00},"BIT 0,L\0" };
		case 0x46: return dis_line{ pc, 2, {0xCB,0x46,0x00},"BIT 0,(HL)\0" };
		case 0x47: return dis_line{ pc, 2, {0xCB,0x47,0x00},"BIT 0,A\0" };
		case 0x48: return dis_line{ pc, 2, {0xCB,0x48,0x00},"BIT 1,B\0" };
		case 0x49: return dis_line{ pc, 2, {0xCB,0x49,0x00},"BIT 1,C\0" };
		case 0x4A: return dis_line{ pc, 2, {0xCB,0x4A,0x00},"BIT 1,D\0" };
		case 0x4B: return dis_line{ pc, 2, {0xCB,0x4B,0x00},"BIT 1,E\0" };
		case 0x4C: return dis_line{ pc, 2, {0xCB,0x4C,0x00},"BIT 1,H\0" };
		case 0x4D: return dis_line{ pc, 2, {0xCB,0x4D,0x00},"BIT 1,L\0" };
		case 0x4E: return dis_line{ pc, 2, {0xCB,0x4E,0x00},"BIT 1,(HL)\0" };
		case 0x4F: return dis_line{ pc, 2, {0xCB,0x4F,0x00},"BIT 1,A\0" };
		case 0x50: return dis_line{ pc, 2, {0xCB,0x50,0x00},"BIT 2,B\0" };
		case 0x51: return dis_line{ pc, 2, {0xCB,0x51,0x00},"BIT 2,C\0" };
		case 0x52: return dis_line{ pc, 2, {0xCB,0x52,0x00},"BIT 2,D\0" };
		case 0x53: return dis_line{ pc, 2, {0xCB,0x53,0x00},"BIT 2,E\0" };
		case 0x54: return dis_line{ pc, 2, {0xCB,0x54,0x00},"BIT 2,H\0" };
		case 0x55: return dis_line{ pc, 2, {0xCB,0x55,0x00},"BIT 2,L\0" };
		case 0x56: return dis_line{ pc, 2, {0xCB,0x56,0x00},"BIT 2,(HL)\0" };
		case 0x57: return dis_line{ pc, 2, {0xCB,0x57,0x00},"BIT 2,A\0" };
		case 0x58: return dis_line{ pc, 2, {0xCB,0x58,0x00},"BIT 3,B\0" };
		case 0x59: return dis_line{ pc, 2, {0xCB,0x59,0x00},"BIT 3,C\0" };
		case 0x5A: return dis_line{ pc, 2, {0xCB,0x5A,0x00},"BIT 3,D\0" };
		case 0x5B: return dis_line{ pc, 2, {0xCB,0x5B,0x00},"BIT 3,E\0" };
		case 0x5C: return dis_line{ pc, 2, {0xCB,0x5C,0x00},"BIT 3,H\0" };
		case 0x5D: return dis_line{ pc, 2, {0xCB,0x5D,0x00},"BIT 3,L\0" };
		case 0x5E: return dis_line{ pc, 2, {0xCB,0x5E,0x00},"BIT 3,(HL)\0" };
		case 0x5F: return dis_line{ pc, 2, {0xCB,0x5F,0x00},"BIT 3,A\0" };
		case 0x60: return dis_line{ pc, 2, {0xCB,0x60,0x00},"BIT 4,B\0" };
		case 0x61: return dis_line{ pc, 2, {0xCB,0x61,0x00},"BIT 4,C\0" };
		case 0x62: return dis_line{ pc, 2, {0xCB,0x62,0x00},"BIT 4,D\0" };
		case 0x63: return dis_line{ pc, 2, {0xCB,0x63,0x00},"BIT 4,E\0" };
		case 0x64: return dis_line{ pc, 2, {0xCB,0x64,0x00},"BIT 4,H\0" };
		case 0x65: return dis_line{ pc, 2, {0xCB,0x65,0x00},"BIT 4,L\0" };
		case 0x66: return dis_line{ pc, 2, {0xCB,0x66,0x00},"BIT 4,(HL)\0" };
		case 0x67: return dis_line{ pc, 2, {0xCB,0x67,0x00},"BIT 4,A\0" };
		case 0x68: return dis_line{ pc, 2, {0xCB,0x68,0x00},"BIT 5,B\0" };
		case 0x69: return dis_line{ pc, 2, {0xCB,0x69,0x00},"BIT 5,C\0" };
		case 0x6A: return dis_line{ pc, 2, {0xCB,0x6A,0x00},"BIT 5,D\0" };
		case 0x6B: return dis_line{ pc, 2, {0xCB,0x6B,0x00},"BIT 5,E\0" };
		case 0x6C: return dis_line{ pc, 2, {0xCB,0x6C,0x00},"BIT 5,H\0" };
		case 0x6D: return dis_line{ pc, 2, {0xCB,0x6D,0x00},"BIT 5,L\0" };
		case 0x6E: return dis_line{ pc, 2, {0xCB,0x6E,0x00},"BIT 5,(HL)\0" };
		case 0x6F: return dis_line{ pc, 2, {0xCB,0x6F,0x00},"BIT 5,A\0" };
		case 0x70: return dis_line{ pc, 2, {0xCB,0x70,0x00},"BIT 6,B\0" };
		case 0x71: return dis_line{ pc, 2, {0xCB,0x71,0x00},"BIT 6,C\0" };
		case 0x72: return dis_line{ pc, 2, {0xCB,0x72,0x00},"BIT 6,D\0" };
		case 0x73: return dis_line{ pc, 2, {0xCB,0x73,0x00},"BIT 6,E\0" };
		case 0x74: return dis_line{ pc, 2, {0xCB,0x74,0x00},"BIT 6,H\0" };
		case 0x75: return dis_line{ pc, 2, {0xCB,0x75,0x00},"BIT 6,L\0" };
		case 0x76: return dis_line{ pc, 2, {0xCB,0x76,0x00},"BIT 6,(HL)\0" };
		case 0x77: return dis_line{ pc, 2, {0xCB,0x77,0x00},"BIT 6,A\0" };
		case 0x78: return dis_line{ pc, 2, {0xCB,0x78,0x00},"BIT 7,B\0" };
		case 0x79: return dis_line{ pc, 2, {0xCB,0x79,0x00},"BIT 7,C\0" };
		case 0x7A: return dis_line{ pc, 2, {0xCB,0x7A,0x00},"BIT 7,D\0" };
		case 0x7B: return dis_line{ pc, 2, {0xCB,0x7B,0x00},"BIT 7,E\0" };
		case 0x7C: return dis_line{ pc, 2, {0xCB,0x7C,0x00},"BIT 7,H\0" };
		case 0x7D: return dis_line{ pc, 2, {0xCB,0x7D,0x00},"BIT 7,L\0" };
		case 0x7E: return dis_line{ pc, 2, {0xCB,0x7E,0x00},"BIT 7,(HL)\0" };
		case 0x7F: return dis_line{ pc, 2, {0xCB,0x7F,0x00},"BIT 7,A\0" };
		case 0x80: return dis_line{ pc, 2, {0xCB,0x80,0x00},"RES 0,B\0" };
		case 0x81: return dis_line{ pc, 2, {0xCB,0x81,0x00},"RES 0,C\0" };
		case 0x82: return dis_line{ pc, 2, {0xCB,0x82,0x00},"RES 0,D\0" };
		case 0x83: return dis_line{ pc, 2, {0xCB,0x83,0x00},"RES 0,E\0" };
		case 0x84: return dis_line{ pc, 2, {0xCB,0x84,0x00},"RES 0,H\0" };
		case 0x85: return dis_line{ pc, 2, {0xCB,0x85,0x00},"RES 0,L\0" };
		case 0x86: return dis_line{ pc, 2, {0xCB,0x86,0x00},"RES 0,(HL)\0" };
		case 0x87: return dis_line{ pc, 2, {0xCB,0x87,0x00},"RES 0,A\0" };
		case 0x88: return dis_line{ pc, 2, {0xCB,0x88,0x00},"RES 1,B\0" };
		case 0x89: return dis_line{ pc, 2, {0xCB,0x89,0x00},"RES 1,C\0" };
		case 0x8A: return dis_line{ pc, 2, {0xCB,0x8A,0x00},"RES 1,D\0" };
		case 0x8B: return dis_line{ pc, 2, {0xCB,0x8B,0x00},"RES 1,E\0" };
		case 0x8C: return dis_line{ pc, 2, {0xCB,0x8C,0x00},"RES 1,H\0" };
		case 0x8D: return dis_line{ pc, 2, {0xCB,0x8D,0x00},"RES 1,L\0" };
		case 0x8E: return dis_line{ pc, 2, {0xCB,0x8E,0x00},"RES 1,(HL)\0" };
		case 0x8F: return dis_line{ pc, 2, {0xCB,0x8F,0x00},"RES 1,A\0" };
		case 0x90: return dis_line{ pc, 2, {0xCB,0x90,0x00},"RES 2,B\0" };
		case 0x91: return dis_line{ pc, 2, {0xCB,0x91,0x00},"RES 2,C\0" };
		case 0x92: return dis_line{ pc, 2, {0xCB,0x92,0x00},"RES 2,D\0" };
		case 0x93: return dis_line{ pc, 2, {0xCB,0x93,0x00},"RES 2,E\0" };
		case 0x94: return dis_line{ pc, 2, {0xCB,0x94,0x00},"RES 2,H\0" };
		case 0x95: return dis_line{ pc, 2, {0xCB,0x95,0x00},"RES 2,L\0" };
		case 0x96: return dis_line{ pc, 2, {0xCB,0x96,0x00},"RES 2,(HL)\0" };
		case 0x97: return dis_line{ pc, 2, {0xCB,0x97,0x00},"RES 2,A\0" };
		case 0x98: return dis_line{ pc, 2, {0xCB,0x98,0x00},"RES 3,B\0" };
		case 0x99: return dis_line{ pc, 2, {0xCB,0x99,0x00},"RES 3,C\0" };
		case 0x9A: return dis_line{ pc, 2, {0xCB,0x9A,0x00},"RES 3,D\0" };
		case 0x9B: return dis_line{ pc, 2, {0xCB,0x9B,0x00},"RES 3,E\0" };
		case 0x9C: return dis_line{ pc, 2, {0xCB,0x9C,0x00},"RES 3,H\0" };
		case 0x9D: return dis_line{ pc, 2, {0xCB,0x9D,0x00},"RES 3,L\0" };
		case 0x9E: return dis_line{ pc, 2, {0xCB,0x9E,0x00},"RES 3,(HL)\0" };
		case 0x9F: return dis_line{ pc, 2, {0xCB,0x9F,0x00},"RES 3,A\0" };
		case 0xA0: return dis_line{ pc, 2, {0xCB,0xA0,0x00},"RES 4,B\0" };
		case 0xA1: return dis_line{ pc, 2, {0xCB,0xA1,0x00},"RES 4,C\0" };
		case 0xA2: return dis_line{ pc, 2, {0xCB,0xA2,0x00},"RES 4,D\0" };
		case 0xA3: return dis_line{ pc, 2, {0xCB,0xA3,0x00},"RES 4,E\0" };
		case 0xA4: return dis_line{ pc, 2, {0xCB,0xA4,0x00},"RES 4,H\0" };
		case 0xA5: return dis_line{ pc, 2, {0xCB,0xA5,0x00},"RES 4,L\0" };
		case 0xA6: return dis_line{ pc, 2, {0xCB,0xA6,0x00},"RES 4,(HL)\0" };
		case 0xA7: return dis_line{ pc, 2, {0xCB,0xA7,0x00},"RES 4,A\0" };
		case 0xA8: return dis_line{ pc, 2, {0xCB,0xA8,0x00},"RES 5,B\0" };
		case 0xA9: return dis_line{ pc, 2, {0xCB,0xA9,0x00},"RES 5,C\0" };
		case 0xAA: return dis_line{ pc, 2, {0xCB,0xAA,0x00},"RES 5,D\0" };
		case 0xAB: return dis_line{ pc, 2, {0xCB,0xAB,0x00},"RES 5,E\0" };
		case 0xAC: return dis_line{ pc, 2, {0xCB,0xAC,0x00},"RES 5,H\0" };
		case 0xAD: return dis_line{ pc, 2, {0xCB,0xAD,0x00},"RES 5,L\0" };
		case 0xAE: return dis_line{ pc, 2, {0xCB,0xAE,0x00},"RES 5,(HL)\0" };
		case 0xAF: return dis_line{ pc, 2, {0xCB,0xAF,0x00},"RES 5,A\0" };
		case 0xB0: return dis_line{ pc, 2, {0xCB,0xB0,0x00},"RES 6,B\0" };
		case 0xB1: return dis_line{ pc, 2, {0xCB,0xB1,0x00},"RES 6,C\0" };
		case 0xB2: return dis_line{ pc, 2, {0xCB,0xB2,0x00},"RES 6,D\0" };
		case 0xB3: return dis_line{ pc, 2, {0xCB,0xB3,0x00},"RES 6,E\0" };
		case 0xB4: return dis_line{ pc, 2, {0xCB,0xB4,0x00},"RES 6,H\0" };
		case 0xB5: return dis_line{ pc, 2, {0xCB,0xB5,0x00},"RES 6,L\0" };
		case 0xB6: return dis_line{ pc, 2, {0xCB,0xB6,0x00},"RES 6,(HL)\0" };
		case 0xB7: return dis_line{ pc, 2, {0xCB,0xB7,0x00},"RES 6,A\0" };
		case 0xB8: return dis_line{ pc, 2, {0xCB,0xB8,0x00},"RES 7,B\0" };
		case 0xB9: return dis_line{ pc, 2, {0xCB,0xB9,0x00},"RES 7,C\0" };
		case 0xBA: return dis_line{ pc, 2, {0xCB,0xBA,0x00},"RES 7,D\0" };
		case 0xBB: return dis_line{ pc, 2, {0xCB,0xBB,0x00},"RES 7,E\0" };
		case 0xBC: return dis_line{ pc, 2, {0xCB,0xBC,0x00},"RES 7,H\0" };
		case 0xBD: return dis_line{ pc, 2, {0xCB,0xBD,0x00},"RES 7,L\0" };
		case 0xBE: return dis_line{ pc, 2, {0xCB,0xBE,0x00},"RES 7,(HL)\0" };
		case 0xBF: return dis_line{ pc, 2, {0xCB,0xBF,0x00},"RES 7,A\0" };
		case 0xC0: return dis_line{ pc, 2, {0xCB,0xC0,0x00},"SET 0,B\0" };
		case 0xC1: return dis_line{ pc, 2, {0xCB,0xC1,0x00},"SET 0,C\0" };
		case 0xC2: return dis_line{ pc, 2, {0xCB,0xC2,0x00},"SET 0,D\0" };
		case 0xC3: return dis_line{ pc, 2, {0xCB,0xC3,0x00},"SET 0,E\0" };
		case 0xC4: return dis_line{ pc, 2, {0xCB,0xC4,0x00},"SET 0,H\0" };
		case 0xC5: return dis_line{ pc, 2, {0xCB,0xC5,0x00},"SET 0,L\0" };
		case 0xC6: return dis_line{ pc, 2, {0xCB,0xC6,0x00},"SET 0,(HL)\0" };
		case 0xC7: return dis_line{ pc, 2, {0xCB,0xC7,0x00},"SET 0,A\0" };
		case 0xC8: return dis_line{ pc, 2, {0xCB,0xC8,0x00},"SET 1,B\0" };
		case 0xC9: return dis_line{ pc, 2, {0xCB,0xC9,0x00},"SET 1,C\0" };
		case 0xCA: return dis_line{ pc, 2, {0xCB,0xCA,0x00},"SET 1,D\0" };
		case 0xCB: return dis_line{ pc, 2, {0xCB,0xCB,0x00},"SET 1,E\0" };
		case 0xCC: return dis_line{ pc, 2, {0xCB,0xCC,0x00},"SET 1,H\0" };
		case 0xCD: return dis_line{ pc, 2, {0xCB,0xCD,0x00},"SET 1,L\0" };
		case 0xCE: return dis_line{ pc, 2, {0xCB,0xCE,0x00},"SET 1,(HL)\0" };
		case 0xCF: return dis_line{ pc, 2, {0xCB,0xCF,0x00},"SET 1,A\0" };
		case 0xD0: return dis_line{ pc, 2, {0xCB,0xD0,0x00},"SET 2,B\0" };
		case 0xD1: return dis_line{ pc, 2, {0xCB,0xD1,0x00},"SET 2,C\0" };
		case 0xD2: return dis_line{ pc, 2, {0xCB,0xD2,0x00},"SET 2,D\0" };
		case 0xD3: return dis_line{ pc, 2, {0xCB,0xD3,0x00},"SET 2,E\0" };
		case 0xD4: return dis_line{ pc, 2, {0xCB,0xD4,0x00},"SET 2,H\0" };
		case 0xD5: return dis_line{ pc, 2, {0xCB,0xD5,0x00},"SET 2,L\0" };
		case 0xD6: return dis_line{ pc, 2, {0xCB,0xD6,0x00},"SET 2,(HL)\0" };
		case 0xD7: return dis_line{ pc, 2, {0xCB,0xD7,0x00},"SET 2,A\0" };
		case 0xD8: return dis_line{ pc, 2, {0xCB,0xD8,0x00},"SET 3,B\0" };
		case 0xD9: return dis_line{ pc, 2, {0xCB,0xD9,0x00},"SET 3,C\0" };
		case 0xDA: return dis_line{ pc, 2, {0xCB,0xDA,0x00},"SET 3,D\0" };
		case 0xDB: return dis_line{ pc, 2, {0xCB,0xDB,0x00},"SET 3,E\0" };
		case 0xDC: return dis_line{ pc, 2, {0xCB,0xDC,0x00},"SET 3,H\0" };
		case 0xDD: return dis_line{ pc, 2, {0xCB,0xDD,0x00},"SET 3,L\0" };
		case 0xDE: return dis_line{ pc, 2, {0xCB,0xDE,0x00},"SET 3,(HL)\0" };
		case 0xDF: return dis_line{ pc, 2, {0xCB,0xDF,0x00},"SET 3,A\0" };
		case 0xE0: return dis_line{ pc, 2, {0xCB,0xE0,0x00},"SET 4,B\0" };
		case 0xE1: return dis_line{ pc, 2, {0xCB,0xE1,0x00},"SET 4,C\0" };
		case 0xE2: return dis_line{ pc, 2, {0xCB,0xE2,0x00},"SET 4,D\0" };
		case 0xE3: return dis_line{ pc, 2, {0xCB,0xE3,0x00},"SET 4,E\0" };
		case 0xE4: return dis_line{ pc, 2, {0xCB,0xE4,0x00},"SET 4,H\0" };
		case 0xE5: return dis_line{ pc, 2, {0xCB,0xE5,0x00},"SET 4,L\0" };
		case 0xE6: return dis_line{ pc, 2, {0xCB,0xE6,0x00},"SET 4,(HL)\0" };
		case 0xE7: return dis_line{ pc, 2, {0xCB,0xE7,0x00},"SET 4,A\0" };
		case 0xE8: return dis_line{ pc, 2, {0xCB,0xE8,0x00},"SET 5,B\0" };
		case 0xE9: return dis_line{ pc, 2, {0xCB,0xE9,0x00},"SET 5,C\0" };
		case 0xEA: return dis_line{ pc, 2, {0xCB,0xEA,0x00},"SET 5,D\0" };
		case 0xEB: return dis_line{ pc, 2, {0xCB,0xEB,0x00},"SET 5,E\0" };
		case 0xEC: return dis_line{ pc, 2, {0xCB,0xEC,0x00},"SET 5,H\0" };
		case 0xED: return dis_line{ pc, 2, {0xCB,0xED,0x00},"SET 5,L\0" };
		case 0xEE: return dis_line{ pc, 2, {0xCB,0xEE,0x00},"SET 5,(HL)\0" };
		case 0xEF: return dis_line{ pc, 2, {0xCB,0xEF,0x00},"SET 5,A\0" };
		case 0xF0: return dis_line{ pc, 2, {0xCB,0xF0,0x00},"SET 6,B\0" };
		case 0xF1: return dis_line{ pc, 2, {0xCB,0xF1,0x00},"SET 6,C\0" };
		case 0xF2: return dis_line{ pc, 2, {0xCB,0xF2,0x00},"SET 6,D\0" };
		case 0xF3: return dis_line{ pc, 2, {0xCB,0xF3,0x00},"SET 6,E\0" };
		case 0xF4: return dis_line{ pc, 2, {0xCB,0xF4,0x00},"SET 6,H\0" };
		case 0xF5: return dis_line{ pc, 2, {0xCB,0xF5,0x00},"SET 6,L\0" };
		case 0xF6: return dis_line{ pc, 2, {0xCB,0xF6,0x00},"SET 6,(HL)\0" };
		case 0xF7: return dis_line{ pc, 2, {0xCB,0xF7,0x00},"SET 6,A\0" };
		case 0xF8: return dis_line{ pc, 2, {0xCB,0xF8,0x00},"SET 7,B\0" };
		case 0xF9: return dis_line{ pc, 2, {0xCB,0xF9,0x00},"SET 7,C\0" };
		case 0xFA: return dis_line{ pc, 2, {0xCB,0xFA,0x00},"SET 7,D\0" };
		case 0xFB: return dis_line{ pc, 2, {0xCB,0xFB,0x00},"SET 7,E\0" };
		case 0xFC: return dis_line{ pc, 2, {0xCB,0xFC,0x00},"SET 7,H\0" };
		case 0xFD: return dis_line{ pc, 2, {0xCB,0xFD,0x00},"SET 7,L\0" };
		case 0xFE: return dis_line{ pc, 2, {0xCB,0xFE,0x00},"SET 7,(HL)\0" };
		case 0xFF: return dis_line{ pc, 2, {0xCB,0xFF,0x00},"SET 7,A\0" };
		}
	case 0xCC: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "CALL Z,$%02x%02x\0", b3, b2); return l; }
	case 0xCD: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "CALL $%02x%02x\0", b3, b2); return l; }
	case 0xCE: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "ADC A,$%02x\0", b2); return l; }
	case 0xCF: return dis_line{ pc, 1, {b1,b2,b3},"RST 08h\0" };
	case 0xD0: return dis_line{ pc, 1, {b1,b2,b3},"RET NC\0" };
	case 0xD1: return dis_line{ pc, 1, {b1,b2,b3},"POP DE\0" };
	case 0xD2: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "JP NC,$%02x%02x\0", b3, b2); return l; }
	case 0xD3: return dis_line{ pc, 1, {b1,b2,b3}, "UNUSED\0" };
	case 0xD4: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "CALL NC,$%02x%02x\0", b3, b2); return l; }
	case 0xD5: return dis_line{ pc, 1, {b1,b2,b3},"PUSH DE\0" };
	case 0xD6: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "SUB A,$%02x\0", b2); return l; }
	case 0xD7: return dis_line{ pc, 1, {b1,b2,b3},"RST 10h\0" };
	case 0xD8: return dis_line{ pc, 1, {b1,b2,b3},"RET C\0" };
	case 0xD9: return dis_line{ pc, 1, {b1,b2,b3},"RETI\0" };
	case 0xDA: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "JP C,$%02x%02x\0", b3, b2); return l; }
	case 0xDB: return dis_line{ pc, 1, {b1,b2,b3}, "UNUSED\0" };
	case 0xDC: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "CALL C,$%02x%02x\0", b3, b2); return l; }
	case 0xDD: return dis_line{ pc, 1, {b1,b2,b3}, "UNUSED\0" };
	case 0xDE: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "SBC A,$%02x\0", b2); return l; }
	case 0xDF: return dis_line{ pc, 1, {b1,b2,b3},"RST 18h\0" };
	case 0xE0: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "LD (FF00+$%02x),A\0", b2); return l; }
	case 0xE1: return dis_line{ pc, 1, {b1,b2,b3},"POP HL\0" };
	case 0xE2: return dis_line{ pc, 1, {b1,b2,b3},"LD (FF00+C),A\0" };
	case 0xE3: return dis_line{ pc, 1, {b1,b2,b3}, "UNUSED\0" };
	case 0xE4: return dis_line{ pc, 1, {b1,b2,b3}, "UNUSED\0" };
	case 0xE5: return dis_line{ pc, 1, {b1,b2,b3},"PUSH HL\0" };
	case 0xE6: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "AND A,$%02x\0", b2); return l; }
	case 0xE7: return dis_line{ pc, 1, {b1,b2,b3},"RST 20h\0" };
	case 0xE8: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "ADD SP,$%02x [%hhi]\0", b2, b2); return l; }
	case 0xE9: return dis_line{ pc, 1, {b1,b2,b3},"JP HL\0" };
	case 0xEA: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "LD ($%02x%02x),A\0", b3, b2); return l; }
	case 0xEB: return dis_line{ pc, 1, {b1,b2,b3}, "UNUSED\0" };
	case 0xEC: return dis_line{ pc, 1, {b1,b2,b3}, "UNUSED\0" };
	case 0xED: return dis_line{ pc, 1, {b1,b2,b3}, "UNUSED\0" };
	case 0xEE: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "XOR A,$%02x\0", b2); return l; }
	case 0xEF: return dis_line{ pc, 1, {b1,b2,b3},"RST 28h\0" };
	case 0xF0: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "LD A,(FF00+$%02x)\0", b2); return l; }
	case 0xF1: return dis_line{ pc, 1, {b1,b2,b3},"POP AF\0" };
	case 0xF2: return dis_line{ pc, 1, {b1,b2,b3},"LD A,(FF00+C)\0" };
	case 0xF3: return dis_line{ pc, 1, {b1,b2,b3},"DI\0" };
	case 0xF4: return dis_line{ pc, 1, {b1,b2,b3}, "UNUSED\0" };
	case 0xF5: return dis_line{ pc, 1, {b1,b2,b3},"PUSH AF\0" };
	case 0xF6: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "OR A,$%02x\0", b2); return l; }
	case 0xF7: return dis_line{ pc, 1, {b1,b2,b3},"RST 30h\0" };
	case 0xF8: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "LD HL,SP+$%02x [%hhi]\0", b2, b2); return l; }
	case 0xF9: return dis_line{ pc, 1, {b1,b2,b3},"LD SP,HL\0" };
	case 0xFA: {dis_line l = dis_line{ pc, 3, {b1,b2,b3},"" }; sprintf(l._text, "LD A,($%02x%02x)\0", b3, b2); return l; }
	case 0xFB: return dis_line{ pc, 1, {b1,b2,b3},"EI\0" };
	case 0xFC: return dis_line{ pc, 1, {b1,b2,b3}, "UNUSED\0" };
	case 0xFD: return dis_line{ pc, 1, {b1,b2,b3}, "UNUSED\0" };
	case 0xFE: {dis_line l = dis_line{ pc, 2, {b1,b2,b3},"" }; sprintf(l._text, "CP A,$%02x\0", b2); return l; }
	case 0xFF: return dis_line{ pc, 1, {b1,b2,b3},"RST 38h\0" };
	}
}