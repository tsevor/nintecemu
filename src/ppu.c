#include "ppu.h"

void ppu_init(PPU *ppu) {
	(void)ppu;
}

bool ppu_clock(PPU *ppu, int cycles) {
	(void)ppu;
	(void)cycles;
	return true; // true if vblank
}
