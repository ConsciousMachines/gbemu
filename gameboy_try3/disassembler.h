// STOP is the only instruction that is >1 byte and has no immediate (aside form CB set)
// CB set has no immediates, and are all 2 bytes each
#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;

struct dis_line
{
	u16 _address;
	int _size;
	u8 _bytes[3];
	char _text[20];
	int _category; // {'control misc' : 0, 'control br' : 1, 'x16 lsm' : 2, 'x8 alu' : 3, 'x8 rsb' : 4, 'x8 lsm' : 5, 'unused' : 6, 'x16 alu' : 7}
};

dis_line disassemble(u8* bytes, u16 pc);