#ifndef C8H
#define C8H

#include <ios>
#include <random>
#include <cstdio>
#include <string>
#include <iostream>

#include <thread>
#include <chrono>
#include <deque>
#include <unordered_map>

using namespace std;


using byte = uint8_t;	// 8bits
using word = uint16_t;	// 16bits





class Chip8 {
private:
	std::array<byte, 4096> m_Memory;
	// byte memory[4*1024] = {0};		// 4KB RAM

	word I = 0;						// Index register
	word PC = 0x200;				// Program Counter / Instruction pointer

	word sp = 0;
	word stack[16] = {0};

	size_t program_size;

public:
	byte DT = 0;					// delay timer
	byte ST = 0;					// sound timer

	byte V[16] = {0};				// CPU registers

	bool key_pressed[16] = {0};
	byte awaitingKey = 0;

	std::array<byte, 2048> m_Screen;

	bool redraw; // draw flag

	void emulate_op();
	bool load_program (const std::string path);

	std::string disassemble();

	Chip8();
};





#endif