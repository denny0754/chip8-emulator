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





class Chip8 {
private:
	std::array<byte, 4096> m_Memory;

	word I = 0;						// Index register
	word PC = 0x200;				// Program Counter / Instruction pointer

	word sp = 0;

	word stack[16] = {0};

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
	// CPU registers
	byte V[16] = {0};

	inline byte GetDelayTimer() const { return m_DelayTimer; }
	
	inline byte GetSoundTimer() const { return m_SoundTimer; }

	void SetDelayTimer(const byte& dt);

	void SetSoundTimer(const byte& st);

	inline bool ShouldRedraw() const { return m_ShouldRedraw; }

	inline void StopDrawing() { m_ShouldRedraw = false; }

	inline std::array<byte, 2048> GetVideoMemory() const { return m_VideoMemory; }

	inline void SetKeyPressed(int index, bool toggle) { m_KeyPressed[index] = toggle; }

	inline void SetAwaitingKey(byte key) { m_AwaitingKey = key; }

	inline byte GetAwaitingKey() const { return m_AwaitingKey; }

	void emulate_op();

	bool load_program (const std::string path);

	std::string disassemble();

	Chip8();
};





#endif