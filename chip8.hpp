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

using byte = uint8_t;	// 8bits
using word = uint16_t;	// 16bits


enum GeneralRegisters
{	
	R0,
	R1,
	R2,
	R3,
	R4,
	R5,
	R6,
	R7,
	R8,
	R9,
	R10,
	R11,
	R12,
	R13,
	R14,
	R15,

	GenRegCount
};

enum InternalRegisters
{
	IR, // Index Register
	PC, // Program Counter
	SP, // Stack Pointer
	VR, // V Register
	IntRegCount
};


class Chip8 {
private:
	std::array<byte, 4096> m_Memory;

	// Internal Registers
	std::array<word, InternalRegisters::IntRegCount> m_InternalRegisters;

	// General Purpose Registers
	std::array<byte, GeneralRegisters::GenRegCount> m_GeneralRegisters;

	std::array<word, 16> m_Stack;

	size_t program_size;

	// Delay Timer
	byte m_DelayTimer = 0;

	// Sound Timer
	byte m_SoundTimer = 0;
	
	std::array<bool, 16> m_KeyPressed;
	
	byte m_AwaitingKey = 0;
	
	std::array<byte, 2048> m_VideoMemory;

	// Should Redraw
	bool m_ShouldRedraw;

public:
	

	inline byte GetDelayTimer() const { return m_DelayTimer; }
	
	inline byte GetSoundTimer() const { return m_SoundTimer; }

	inline void SetDelayTimer(const byte& dt) { m_DelayTimer = dt; }

	inline void SetSoundTimer(const byte& st) { m_SoundTimer = st; }

	inline bool ShouldRedraw() const { return m_ShouldRedraw; }

	inline void StopDrawing() { m_ShouldRedraw = false; }

	inline std::array<byte, 2048> GetVideoMemory() const { return m_VideoMemory; }

	inline void SetKeyPressed(int index, bool toggle) { m_KeyPressed[index] = toggle; }

	inline void SetAwaitingKey(byte key) { m_AwaitingKey = key; }

	inline byte GetAwaitingKey() const { return m_AwaitingKey; }

	inline byte GetGeneralRegister(GeneralRegisters reg) const { return m_GeneralRegisters[reg]; }

	inline void SetGeneralRegister(GeneralRegisters reg, byte val) { m_GeneralRegisters[reg] = val; }

	void emulate_op();

	bool load_program (const std::string path);

	std::string disassemble();

	Chip8();
};





#endif