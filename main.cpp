#include "main.h"
#include "TMS9995.h"
#include "TMS9919.h"
#include <getopt.h>
#include <time.h>
#include <algorithm>
#include <string>
#include <vector>
#include <SDL2/SDL.h>

#define KEY			m[0xfffb]
#define KEY_UP		1
#define KEY_DOWN	2
#define KEY_LEFT	4
#define KEY_RIGHT	8
#define KEY_A		16
#define KEY_B		32

static uint8_t m[0x10000];
static TMS9995 cpu;
static TMS9919 dcsg;
static int mhz, vramAdr;
static bool exit_flag;
static SDL_GameController *controller;

void emu_exit() {
	exit_flag = true;
}

void set_vram(int adr) {
	vramAdr = adr;
}

void set_dcsg(int data) {
	dcsg.Set(data);
}

struct Sym {
	Sym(int _adr, const char *_s = "") : adr(_adr), n(0), s(_s) {}
	int adr, n;
	std::string s;
};

static std::vector<Sym> sym;

static bool cmp_adr(const Sym &a, const Sym &b) { return a.adr < b.adr; }
static bool cmp_n(const Sym &a, const Sym &b) { return a.n > b.n; }

static void dumpProfile() {
	if (sym.empty()) return;
	std::sort(sym.begin(), sym.end(), cmp_n);
	int t = 0;
	for (auto i = sym.begin(); i != sym.end() && i->n; i++) t += i->n;
	printf("------\n");
	for (auto i = sym.begin(); i != sym.end() && double(i->n) / t >= 1e-2; i++)
		printf("%4.1f%% %s", 100. * i->n / t, i->s.c_str());
	sym.clear();
}

static SDL_AudioSpec audiospec;
static void audio_callback(void *userdata, uint8_t *stream, int len) {
	cpu.Execute(1e6 * mhz * double(len / 4) / audiospec.freq);
	if (!sym.empty()) {
		auto it = upper_bound(sym.begin(), sym.end(), Sym(cpu.GetPC()), cmp_adr);
		if (it != sym.begin() && it != sym.end()) it[-1].n++;
	}
	float *snd = (float *)stream;
	for (int i = 0; i < len / 4; i++) snd[i] = dcsg.Update();
}

static void audio_init() {
	SDL_Init(SDL_INIT_AUDIO);
	SDL_AudioSpec spec;
	SDL_zero(spec);
	spec.freq = 48000;
	spec.format = AUDIO_F32SYS;
	spec.channels = 1;
	spec.samples = 512;
	spec.callback = audio_callback;
	int id = SDL_OpenAudioDevice(nullptr, 0, &spec, &audiospec, 0);
	if (id <= 0) exit(1);
	SDL_PauseAudioDevice(id, 0);
}

static void exitfunc() {
	dumpProfile();
	SDL_Quit();
}

int main(int argc, char *argv[]) {
	int c;
	mhz = 100;
	while ((c = getopt(argc, argv, "c:")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &mhz);
				break;
		}
	if (argc <= optind) {
		fprintf(stderr, "Usage: emu9995 [-c <clock freq. [MHz]> (default: %d)] <a.out file>\n", mhz);
		return 1;
	}
	char path[256];
	strcpy(path, argv[optind]);
	FILE *fi = fopen(path, "r");
	if (!fi) {
		fprintf(stderr, "Cannot open %s\n", path);
		return 1;
	}
	uint16_t i = 0;
	while ((c = getc(fi)) != EOF) m[i++] = c;
	fclose(fi);
	fi = nullptr;
	char *p = strrchr(path, '.');
	if (p) {
		strcpy(p, ".adr");
		fi = fopen(path, "r");
		if (fi) {
			char s[256];
			while (fgets(s, sizeof(s), fi)) {
				int adr;
				if (sscanf(s, "%x", &adr) == 1 && strlen(s) > 5) sym.emplace_back(adr, s + 5);
			}
			fclose(fi);
			sym.emplace_back(0xffff);
		}
	}
	m[1] = 0x80; // level0 wp: 0x80
	m[2] = 1; // level0 pc: 0x100
	m[5] = 0xc0; // level1 wp: 0xc0
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	(uint32_t &)m[0xfff4] = (uint32_t)ts.tv_sec; // for srand
	cpu.SetMemoryPtr(m);
	cpu.Reset();
	atexit(exitfunc);
	//
	audio_init();
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
	if (SDL_NumJoysticks() && SDL_IsGameController(0))
		controller = SDL_GameControllerOpen(0);
	constexpr int width = 96, height = 160, mag = 4;
	SDL_Window *window = SDL_CreateWindow("emu9995", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, mag * width, mag * height, 0);
	if (!window) exit(1);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) exit(1);
	SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(SDL_SWSURFACE, width, height, 1, SDL_PIXELFORMAT_INDEX1MSB);
	if (!surface) exit(1);
	static constexpr SDL_Color colors[] = { { 0, 0, 0, 255 }, { 255, 255, 255, 255 } };
	SDL_SetPaletteColors(surface->format->palette, colors, 0, 2);
	int x = 0, y = 0, pitch = width / 8;
	while (!exit_flag) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
            static SDL_Keycode sym;
			switch (e.type) {
				case SDL_KEYDOWN:
					if (e.key.repeat) break;
					switch (sym = e.key.keysym.sym) {
						case SDLK_RIGHT: KEY |= KEY_RIGHT; break;
                        case SDLK_LEFT:  KEY |= KEY_LEFT;  break;
                        case SDLK_UP:    KEY |= KEY_UP;    break;
                        case SDLK_DOWN:  KEY |= KEY_DOWN;  break;
						case SDLK_x:     KEY |= KEY_A;     break;
						case SDLK_z:     KEY |= KEY_B;     break;
					}
					break;
				case SDL_KEYUP:
					switch (sym = e.key.keysym.sym) {
						case SDLK_RIGHT: KEY &= ~KEY_RIGHT; break;
						case SDLK_LEFT:  KEY &= ~KEY_LEFT;  break;
						case SDLK_UP:    KEY &= ~KEY_UP;    break;
						case SDLK_DOWN:  KEY &= ~KEY_DOWN;  break;
						case SDLK_x:     KEY &= ~KEY_A;     break;
						case SDLK_z:     KEY &= ~KEY_B;     break;
					}
 					break;
				case SDL_QUIT:
					exit_flag = true;
					break;
			}
		}
		if (controller) {
			auto f = [](int mask, auto x) {
				if (SDL_GameControllerGetButton(controller, x)) KEY |= mask;
				else KEY &= ~mask;
			};
			f(KEY_RIGHT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
			f(KEY_LEFT, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
			f(KEY_UP, SDL_CONTROLLER_BUTTON_DPAD_UP);
			f(KEY_DOWN, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
			f(KEY_A, SDL_CONTROLLER_BUTTON_B);
			f(KEY_B, SDL_CONTROLLER_BUTTON_A);
		}
		for (y = 0; y < height; y++)
			for (x = 0; x < pitch; x++)
				((char *)surface->pixels)[surface->pitch * y + x] = m[vramAdr + pitch * y + x];
		SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_RenderCopy(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);
		SDL_DestroyTexture(texture);
		cpu.INT1();
	}
	return 0;
}
