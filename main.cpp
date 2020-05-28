/**
 * Chip8 Emulator
 * 
 * (C) Sochima Biereagu, 2019
*/

#include <iostream>
#include "chip8.hpp"
#include <SDL2/SDL.h>
#include <cxxopts/cxxopts.hpp>

std::string getfilename(const std::string& filepath);


// chip8 screen size
const int w = 64, h = 32;

// emulator window size
const int W = w*16, H = h*16;

// RGB buffer
uint32_t pixels[w * h];

// render screen in RGB buffer
void renderTo(uint32_t* pixels, const std::array<byte, 2048>& screen)
{
	for (int i = 0; i < w * h; ++i)
	{
		pixels[i] = (0x0033FF66 * screen[i]) | 0xFF111111;
	}
}

static std::deque<std::pair<unsigned,bool>> AudioQueue;


#undef main
int main(int argc, char** argv)
{
	// Using library `cxxopts` to parse arguments passed to the program.
	cxxopts::Options options("Chip8 Emulator", "An emulator for the Chip8.");

	options.add_options()
        ("f,file", "Block Size of the Disk(In Kilobytes - Minumum is 512)", cxxopts::value<std::string>())
        ("d,decode", "Size of the Disk in MB(MegaBytes)", cxxopts::value<bool>()->default_value("false"))
		("h,help", "Prints this.");

	auto arg_result = options.parse(argc, argv);

	Chip8 emulator = Chip8();

	// We need at least a ROM file to start the emulator.
	if(arg_result.count("help") || arg_result.arguments().empty())
	{
		std::cout << options.help() << std::endl;
		exit(0);
	}

	std::string file_path = std::string();
	bool decode_rom = false;

	try
	{
		file_path = arg_result["file"].as<std::string>();
		decode_rom = arg_result["decode"].as<bool>();
	}catch(...) {  }

	if(!file_path.empty() && !emulator.load_program(file_path))
	{
		std::cerr << "Couldn't load ROM `" << file_path << "`.\n";
		exit(-1);
	}
	if(decode_rom)
	{
		std::cout << emulator.disassemble();
	}

	std::cout << "\nPress P to pause emulation.\n";


	/////////
	// GUI //
	/////////

	std::string filename = getfilename(file_path);

	// create window
	std::string title = "Chip8 Emulator - " + filename;
	SDL_Window* window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W, H, SDL_WINDOW_RESIZABLE);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0); SDL_RenderSetLogicalSize(renderer, W, H);
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);

	// mapping of SDL keyboard symbols to chip8 keypad codes
	std::unordered_map<int,int> keymap{
		{SDLK_1, 0x1}, {SDLK_2, 0x2}, {SDLK_3, 0x3}, {SDLK_4, 0xC},
		{SDLK_q, 0x4}, {SDLK_w, 0x5}, {SDLK_e, 0x6}, {SDLK_r, 0xD},
		{SDLK_a, 0x7}, {SDLK_s, 0x8}, {SDLK_d, 0x9}, {SDLK_f, 0xE},
		{SDLK_z, 0xA}, {SDLK_x, 0x0}, {SDLK_c, 0xB}, {SDLK_v, 0xF},
		{SDLK_5, 0x5}, {SDLK_6, 0x6}, {SDLK_7, 0x7}, {SDLK_p, -2},
		{SDLK_8, 0x8}, {SDLK_9, 0x9}, {SDLK_0, 0x0}, {SDLK_ESCAPE,-1}
	};

	// Init. SDL audio
	SDL_AudioSpec spec, obtained;
	spec.freq     = 44100;
	spec.format   = AUDIO_S16SYS;
	spec.channels = 2;
	spec.samples  = spec.freq/20;
	spec.callback = [](void*, Uint8* stream, int len) {
		short* target = (short*)stream;
		while(len > 0 && !AudioQueue.empty())
		{
			auto& data = AudioQueue.front();
			for(; len && data.first; target += 2, len -= 4, --data.first)
			{
				target[0] = target[1] = data.second*300*((len&128)-64);
			}
			if(!data.first) AudioQueue.pop_front();
		}
	};
	SDL_OpenAudio(&spec, &obtained);
	SDL_PauseAudio(0);


	int msg = 0;
	int frames_done = 0;
	bool running = true,
		 paused = false;


	auto start = std::chrono::system_clock::now();

	while (running)
	{
		/////////////////////////
		// Execute Instruction //
		/////////////////////////
		if (!emulator.GetAwaitingKey() && !paused)
			// if not waiting for input nor paused
			emulator.emulate_op();

		if (paused && !msg) {
			printf("\nPaused\n");
			msg = 1;
		}

		////////////////////
		// Process events //
		////////////////////
		for(SDL_Event ev; SDL_PollEvent(&ev); )
		{
			switch(ev.type)
			{
				case SDL_QUIT:
				{
					running = 0;
					break;
				}
				case SDL_KEYDOWN:
				case SDL_KEYUP:
				{
					auto key = keymap.find(ev.key.keysym.sym);

					if (key == keymap.end()) break;
					if (key->second == -1) { running = 0; break; }
					if (key->second == -2 && ev.type==SDL_KEYDOWN) {
						paused=!paused, msg=0;
						if (!paused)
						{
							std::cout << "Resumed\n";
						}
						break;
					}

					emulator.SetKeyPressed(key->second, (ev.type == SDL_KEYDOWN));

					if(ev.type==SDL_KEYDOWN && emulator.GetAwaitingKey())
					{
						emulator.V[emulator.GetAwaitingKey() & 0x7f] = key->second;
						emulator.SetAwaitingKey(0);
					}
				}
			}
		}

		//////////////
		// Render  //
		/////////////
		auto cur = std::chrono::system_clock::now();

		std::chrono::duration<double> elapsed_seconds = cur-start;
		int frames = int(elapsed_seconds.count() * 60) - frames_done;

		if (frames > 0 && !paused) {
			frames_done += frames;

            // Update the timer registers
            int st = std::min<int>(frames, emulator.GetSoundTimer());
			emulator.SetSoundTimer(emulator.GetSoundTimer() - st);
			int dt = std::min<int>(frames, emulator.GetDelayTimer());
			emulator.SetDelayTimer(emulator.GetDelayTimer() - dt);

            //////////////////
            // Render audio //
            SDL_LockAudio();
             AudioQueue.emplace_back( obtained.freq*st/60,  true );
             AudioQueue.emplace_back( obtained.freq*(frames-st)/60, false );
            SDL_UnlockAudio();

			/////////////////////
			// Render graphics //
			if (emulator.ShouldRedraw()) {
				renderTo(pixels, emulator.GetVideoMemory());
				SDL_UpdateTexture(texture, nullptr, pixels, 4*w);
				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, nullptr, nullptr);
				SDL_RenderPresent(renderer);

				emulator.StopDrawing();
			}
		}

		if (!paused)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(1300));
		}
	}

	return 0;
}


/**
 * get filename from filepath
 */
std::string getfilename(const std::string& filepath) {
	std::string filename = filepath;

	// Remove directory if present.
	// Do this before extension removal incase directory has a period character.
	const size_t last_slash_idx = filename.find_last_of("\\/");
	if (std::string::npos != last_slash_idx)
		filename.erase(0, last_slash_idx + 1);

	// Remove extension if present.
	const size_t period_idx = filename.rfind('.');
	if (std::string::npos != period_idx)
	    filename.erase(period_idx);

	return filename;
}
