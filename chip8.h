#ifndef C8H
#define C8H

#include <ios>
#include <cstdio>
#include <string.h>
#include <stdint.h>
#include <iostream>

using namespace std;


using byte = uint8_t;	// 8bits
using word = uint16_t;	// 16bits


byte chip8_fonts[5*16] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};


class Chip8 {
private:
	byte memory[4*1024] = {0};		// 4KB RAM
	byte V[16] = {0};				// 8bit registers

	word I = 0;							// Index register
	word PC = 0x200;				// program Counter / Instruction pointer

	word stack[16] = {0};			// Stack
	word sp = 0;					// Stack pointer

	byte DT = 0;					// delay timer
	byte ST = 0;					// sound timer

	size_t program_size;

public:
	bool key_pressed[16];
	byte screen[64*32] = {0};

	void emulate_op();
	bool load_program (std::string file);

	std::string disassemble (void);

	Chip8();
};


// init
Chip8::Chip8() {

	// load fonts into memory
	memcpy(memory, chip8_fonts, 5*16);

}


#endif