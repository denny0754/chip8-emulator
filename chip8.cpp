/**
 * Chip8 Emulator
 * 
 * chip8.cc
 * 
 * 	Contains code to load a program,
 *   and the instruction interpreter.
 *
 * (C) Sochima Biereagu, 2019
*/

#include "chip8.hpp"
#include <vector>
#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>
#include <random>
#include <spdlog/sinks/stdout_color_sinks.h>


static std::array<byte, 80> CHIP8_FONTS =
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

// init
Chip8::Chip8()
{

	// load fonts into memory
	std::copy(CHIP8_FONTS.begin(), CHIP8_FONTS.end(), m_Memory.begin());
	m_VideoMemory.fill(0);
	m_GeneralRegisters.fill(0);
	m_InternalRegisters.fill(0);
	m_InternalRegisters[InternalRegisters::PC] = 0x200;
	m_Stack.fill(0);

	auto color_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	m_Logger.reset(new spdlog::logger("CHIP8", color_sink));
    m_Logger->set_level(spdlog::level::trace);
    m_Logger->set_pattern("[%T][%l] %n: %^%v%$");
}

/*

	static std::size_t GetFileSize

	Param const std::string& path: Path to the file.

	Returns the size of the given file.
	If the file can't be opened(not enough permissios) 0 is returned.
	Otherwise, its size is returned.

*/
static std::size_t GetFileSize(const std::string& path)
{
	std::ifstream file_buffer = std::ifstream(path,std::ios::in|std::ios::binary);
	if(!file_buffer.good())
	{
		return 0;
	}

	file_buffer.ignore( std::numeric_limits<std::streamsize>::max() );
	std::streamsize length = file_buffer.gcount();
	file_buffer.clear();   //  Since ignore will have set eof.
	file_buffer.seekg( 0, std::ios_base::beg );
	file_buffer.close();

	return length;
}

/**
 * Load ROM into memory
 */
bool Chip8::load_program(const std::string file)
{
	program_size = GetFileSize(file);
	if(!program_size)
	{
		m_Logger->error("Program file '{}' is empty.", file);
		return false;
	}

	std::fstream file_buffer = std::fstream(file, std::ios::in | std::ios::binary);
	if (!file_buffer.good()) 
	{
		m_Logger->error("Couldn't open '{}'. Not enough permissions.", file);
		return false;
	}

	// read program into buffer
	std::stringstream sbuffer;
	sbuffer << file_buffer.rdbuf();
	std::string bstr = sbuffer.str();
	
	// load program from buffer into memory
	for(std::size_t i = 0; i < program_size; i++)
	{
		m_Memory[i + m_InternalRegisters[InternalRegisters::PC]] = (byte)bstr[i];
	}

	file_buffer.close();
	return true;
}


/**
 * Interprete an instruction
 *  a single cycle
 *
 * See chip8 instruction set at http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.1
 */
void Chip8::emulate_op()
{
	#define not_handled(m,l) printf("\nUnrecognized instruction: %04x %04x\n", m,l); exit(2);
	
	std::mt19937 rnd{};

	int tmp;
	int opcode = (m_Memory[m_InternalRegisters[InternalRegisters::PC]] << 8) | m_Memory[m_InternalRegisters[InternalRegisters::PC]+1];
	int msb = opcode>>8, lsb = opcode&0xff;

	// Get bit-fields from instruction/opcode
	int u   = (opcode>>12) & 0xF;
	int x   = (opcode>>8) & 0xF;
	int y   = (opcode>>4) & 0xF;
	int kk  = (opcode) & 0xFF;
	int n   = (opcode) & 0xF;
	int nnn = (opcode) & 0xFFF;

	// alliases for common registers
	byte &Vx = m_GeneralRegisters[x];
	byte &Vy = m_GeneralRegisters[y];
	byte &VF = m_GeneralRegisters[0xF];

	m_Logger->debug("Current Instruction: {}", decode(m_InternalRegisters[InternalRegisters::PC], msb, lsb));

	switch (msb >> 4) {
		// 0x0e-
		case 0x0: {
			switch (lsb&0xf) {
				// 0x00e0
				case 0x0: // clr

					break;

				// 0x00ee
				case 0xe: // ret
					m_InternalRegisters[InternalRegisters::PC] = m_Stack[--m_InternalRegisters[InternalRegisters::SP]] + 2;
					break;

				default:
					not_handled(msb, lsb);
			}
		}
		break;

		// 0x1nnn
		case 0x1: // jmp nnn
			m_InternalRegisters[InternalRegisters::PC] = nnn;
			break;

		// 0x2nnn
		case 0x2: // call nnn
			m_Stack[m_InternalRegisters[InternalRegisters::SP]++] = m_InternalRegisters[InternalRegisters::PC];
			m_InternalRegisters[InternalRegisters::PC] = nnn;
			break;

		// 03xkk
		case 0x3: // jeq Vx, Vkk
			if (Vx == kk) m_InternalRegisters[InternalRegisters::PC] += 2;
			// Although this emulator forces a thread sleep(this_thread::sleep) of 1300 milliseconds,
			// theoretically, removing the `if` statement should improve performance as it
			// removes a branch instruction from the compiled version.
			// Basically, if we add `2 * (Vx == kk)` to the program counter,
			// depending on the situation, we would add 4 or 2 to it.
			m_InternalRegisters[InternalRegisters::PC] += 2;// + (2 * (Vx == kk));
			break;

		// 0x4xkk
		case 0x4: //jneq Vx, Vkk
			if (Vx != kk) m_InternalRegisters[InternalRegisters::PC] += 2;
			m_InternalRegisters[InternalRegisters::PC] += 2;
			break;

		// 0x5xy0
		case 0x5: // jeqr Vx, Vy
			if (Vx == Vy) m_InternalRegisters[InternalRegisters::PC] += 2;
			m_InternalRegisters[InternalRegisters::PC] += 2;
			break;

		// 0x6xkk
		case 0x6: // mov Vx, kk
			Vx = kk;
			m_InternalRegisters[InternalRegisters::PC] += 2;
			break;

		// 0x7xkk
		case 0x7: // add Vx, kk
            Vx += kk;
			m_InternalRegisters[InternalRegisters::PC] += 2;
			break;

		// 0x8xyn
		case 0x8: {
			switch (lsb & 0xf) {
				// 0x8xy0
				case 0x0: // mov Vx, Vy
					Vx = Vy;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0x8xy1
				case 0x1: // or Vx, Vy
					Vx |= Vy;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0x8xy2
				case 0x2: // and Vx, Vy
					Vx &= Vy;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0x8xy3
				case 0x3: // xor Vx, Vy
					Vx ^= Vy;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0x8xy4
				case 0x4: // addr Vx, Vy
					tmp = Vx + Vy;
					VF = (tmp > 0xff);
					Vx += Vy;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0x8xy5
				case 0x5: // sub Vx, Vy
					VF = Vx > Vy;
					Vx -= Vy;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0x8xy6
				case 0x6: // shr Vx
					VF = Vx&1;
					Vx >>= 1;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0x8xy7
				case 0x7: // subb Vy, Vx
					VF = Vy > Vx;
					Vy -= Vx;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0x8ye
				case 0xe: // shl Vx
					VF = Vx >> 7;
					Vx <<= 1;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				default:
					not_handled(msb, lsb);
			}
		}
		break;

		// 9xy0
		case 0x9: // jneqr Vx, Vy
			if (Vx != Vy) m_InternalRegisters[InternalRegisters::PC] += 2;
			m_InternalRegisters[InternalRegisters::PC] += 2;
			break;

		// Annn
		case 0xA: // mov I, nnn
			m_InternalRegisters[InternalRegisters::IR] = nnn;
			m_InternalRegisters[InternalRegisters::PC] += 2;
			break;

		// 0xBnnn
		case 0xB: // jmp V0+nnn
			m_InternalRegisters[InternalRegisters::PC] = m_GeneralRegisters[GeneralRegisters::R0] + nnn;
			break;

		// 0xCxkk
		case 0xC: // rnd Vx, kk
			Vx = std::uniform_int_distribution<>(0, 255)(rnd) & kk;
			m_InternalRegisters[InternalRegisters::PC] += 2;
			break;

		// 0xDxyn
		case 0xD: { // draw Vx, Vy, n
			/*
				Read n bytes from memory, starting at the address stored in I.
				These bytes are then displayed as sprites on screen at coordinates (Vx, Vy), with width = 8 and height = n.
				Sprites are XORed onto the existing screen. If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0.
				If the sprite is positioned so part of it is outside the coordinates of the display, it wraps around to the opposite side of the screen.
			*/
			byte width = 8,
				height = n,
				*row_pixels = &m_Memory[m_InternalRegisters[InternalRegisters::IR]];

			int px, py;
			for (int h = 0; h < height; ++h) {
				py = (Vy + h) % 32;

				for (int w = 0; w < width; ++w) {
					px = (Vx + w) % 64;

					if (*row_pixels & (0b10000000 >> w)) { // checks if (8-w)'th bit is set
						byte &pixel = m_VideoMemory[64*py + px];

						VF = (pixel == 1),
						pixel ^= 1;
					}
				}
				++row_pixels;
			}
			m_ShouldRedraw = true;
			m_InternalRegisters[InternalRegisters::PC] += 2;
		}
		break;

		// 0xEx--
		case 0xE: {
			switch (lsb) {
				// 0xE9E
				case 0x9E: // jkey Vx
					if (m_KeyPressed[Vx]) m_InternalRegisters[InternalRegisters::PC] += 2;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0xA1
				case 0xA1: // jnkey Vx
					if (!m_KeyPressed[Vx]) m_InternalRegisters[InternalRegisters::PC] += 2;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;
			}
		}
		break;

		// 0xFx--
		case 0xF: {
			switch (lsb) {
				// 0xFx07
				case 0x07: // getdelay Vx
					Vx = m_DelayTimer;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0xFx0A
				case 0x0A: // waitkey Vx
					m_AwaitingKey = 0x80 | x;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0xFx15
				case 0x15: // setdelay Vx
					m_DelayTimer = Vx;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0xFx18
				case 0x18: // setsound Vx
					m_SoundTimer = Vx;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0xFx1E
				case 0x1E: // mov I, Vx
					m_InternalRegisters[InternalRegisters::IR] += Vx;
					VF = (m_InternalRegisters[InternalRegisters::IR] + Vx > 0xff);
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0xFx29
				case 0x29: // spritei I, Vx
					m_InternalRegisters[InternalRegisters::IR] = 5 * Vx;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0xFx33
				case 0x33: // bcd [I], Vx
					tmp = Vx;
					byte a,b,c;
					c = tmp%10, tmp/=10;
					b = tmp%10, tmp/=10;
					a = tmp;

					m_Memory[m_InternalRegisters[InternalRegisters::IR]] = a;
					m_Memory[m_InternalRegisters[InternalRegisters::IR] + 1] = b;
					m_Memory[m_InternalRegisters[InternalRegisters::IR] + 2] = c;
					m_InternalRegisters[InternalRegisters::PC] += 2;
					break;

				// 0xFx55
				case 0x55: { // mov [I], V0-VF
					byte *memv = &m_Memory[m_InternalRegisters[InternalRegisters::IR]];

					for (int p = 0; p <= x; ++p)
						*(memv++) = m_GeneralRegisters[p];

					m_InternalRegisters[InternalRegisters::IR] += x + 1;
					m_InternalRegisters[InternalRegisters::PC] += 2;
				}
				break;

				// 0xFx65
				case 0x65: { // mov V0-VF, [I]
					byte *memv = &m_Memory[m_InternalRegisters[InternalRegisters::IR]];

					for (int p = 0; p <= x; ++p)
						m_GeneralRegisters[p] = *(memv++);

					m_InternalRegisters[InternalRegisters::IR] += x+1, m_InternalRegisters[InternalRegisters::PC] += 2;
					break;
				}

				default:
					not_handled(msb, lsb);
			}
			break;
		}

		default:
			not_handled(msb, lsb);
	}
	#undef not_handled
}



/*

Converts program from memory back to Chip8 instructions.

Chip8 doesnt have an assembly instruction set,
so i'm just going to use something similar to NASM.

The generated code isnt neccessarily valid, 
its just used for debugging purpose.

*/
std::string Chip8::disassemble() {

	byte lsb, msb;
	std::string str = "";

	for (int i=PC; i < (PC + program_size); i+=2) {
		msb = m_Memory[i], lsb = m_Memory[i+1];

		str += decode(i, msb, lsb) + "\n";
	}

	return str;
}




#define ADDR ((msb&0xf)<<8)|lsb // address
// 0xf = 0b1111
// decode instruction
std::string Chip8::decode(int i, byte msb, byte lsb) {
	char ln[20];
	sprintf(ln, "%04x:  %02x %02x  =>  ", i, msb, lsb);
	std::string str = ln;

	byte nib = msb >> 4;

	switch (nib)
	{
		case 0x0: 
			switch (lsb)
			{
				case 0xe0: {sprintf(ln,"%s","clear"), str+=ln;} break;
				case 0xee: {sprintf(ln,"%s","ret"), str+=ln;} break;
			}
			break;
		case 0x1: {sprintf(ln,"%s 0x%03x", "jmp", ADDR), str+=ln;} break;
		case 0x2: {sprintf(ln,"%s 0x%02x", "call", ADDR), str+=ln;} break;
		case 0x3: {sprintf(ln,"%s V%01X, 0x%02x", "jeq", msb&0xf, lsb), str+=ln;} break;
		case 0x4: {sprintf(ln,"%s V%01X, 0x%02x", "jneq", msb&0xf, lsb), str+=ln;} break;
		case 0x5: {sprintf(ln,"%s V%01X, V%01X", "jeqr", msb&0xf, lsb>>4), str+=ln;} break;
		case 0x6: {sprintf(ln,"%s V%01X, 0x%02x", "mov", msb&0xf, lsb), str+=ln;} break;
		case 0x7: {sprintf(ln,"%s V%01X, 0x%02x", "add", msb&0xf, lsb), str+=ln;} break;
		case 0x8:
			{
				byte lastnib = lsb&0xf;
				switch(lastnib)
				{
					case 0: {sprintf(ln,"%s V%01X, V%01X", "mov", msb&0xf, lsb>>4), str+=ln;} break;
					case 1: {sprintf(ln,"%s V%01X, V%01X", "or", msb&0xf, lsb>>4), str+=ln;} break;
					case 2: {sprintf(ln,"%s V%01X, V%01X", "and", msb&0xf, lsb>>4), str+=ln;} break;
					case 3: {sprintf(ln,"%s V%01X, V%01X", "xor", msb&0xf, lsb>>4), str+=ln;} break;
					case 4: {sprintf(ln,"%s V%01X, V%01X", "addr", msb&0xf, lsb>>4), str+=ln;} break;
					case 5: {sprintf(ln,"%s V%01X, V%01X", "sub", msb&0xf, lsb>>4), str+=ln;} break;
					case 6: {sprintf(ln,"%s V%01X, V%01X", "shr", msb&0xf, lsb>>4), str+=ln;} break;
					case 7: {sprintf(ln,"%s V%01X, V%01X", "subb", msb&0xf, lsb>>4), str+=ln;} break;
					case 0xe: {sprintf(ln,"%s V%01X, V%01X", "shl", msb&0xf, lsb>>4), str+=ln;} break;
				}
			}
			break;
		case 0x9: {sprintf(ln,"%s V%01X, V%01X", "jneqr", msb&0xf, lsb>>4), str+=ln;} break;
		case 0xa: {sprintf(ln,"%s I, [0x%03x]", "mov", ADDR), str+=ln;} break;
		case 0xb: {sprintf(ln,"%s 0x%03x+(V0)", "jmp", ADDR), str+=ln;} break;
		case 0xc: {sprintf(ln,"%s V%01X, 0x%02X", "rand", msb&0xf, lsb), str+=ln;} break;
		case 0xd: {sprintf(ln,"%s V%01X, V%01X, 0x%01x", "draw", msb&0xf, lsb>>4, lsb&0xf), str+=ln;} break;
		case 0xe: 
			switch(lsb)
			{
				case 0x9E: {sprintf(ln,"%s V%01X", "jkey", msb&0xf), str+=ln;} break;
				case 0xA1: {sprintf(ln,"%s V%01X", "jnkey", msb&0xf), str+=ln;} break;
			}
			break;
		case 0xf: 
			switch(lsb)
			{
				case 0x07: {sprintf(ln,"%s V%01X", "getdelay", msb&0xf), str+=ln;} break;
				case 0x0a: {sprintf(ln,"%s V%01X", "waitkey", msb&0xf), str+=ln;} break;
				case 0x15: {sprintf(ln,"%s V%01X", "setdelay", msb&0xf), str+=ln;} break;
				case 0x18: {sprintf(ln,"%s V%01X", "setsound", msb&0xf), str+=ln;} break;
				case 0x1e: {sprintf(ln,"%s I, V%01X", "mov", msb&0xf), str+=ln;} break;
				case 0x29: {sprintf(ln,"%s I, V%01X", "spritei", msb&0xf), str+=ln;} break;
				case 0x33: {sprintf(ln,"%s [I], V%01X", "bcd", msb&0xf), str+=ln;} break;
				case 0x55: {sprintf(ln,"%s [I], V0-V%01X", "mov", msb&0xf), str+=ln;} break;
				case 0x65: {sprintf(ln,"%s V0-V%01X, [I]", "mov", msb&0xf), str+=ln;} break;
			}
			break;

			default: {sprintf(ln,"unknown"), str+=ln;} break;
	}

	return str;
}

