#include "ppu.h"

#include <stdlib.h>
#include "host.h"

#define NES_WIDTH 256
#define NES_HEIGHT 240

void ppu_init(PPU *ppu) {
	ppu->buffer = malloc(NES_WIDTH * NES_HEIGHT * sizeof(uint32_t));

	ppu->scanline = 0;
	ppu->dot = 0;
}

bool ppu_clock(PPU *ppu, int cycles) {
	(void)cycles;

	// test draw
	uint8_t r1 = 255;
	uint8_t g1 = 0;
	uint8_t b1 = 0;

	uint8_t r2 = 0;
	uint8_t g2 = 0;
	uint8_t b2 = 255;

	for (int i = 0; i < NES_WIDTH * NES_HEIGHT; i++) {
		double lerpprog = ((double)i / (double)(NES_WIDTH * NES_HEIGHT));
		uint8_t lr = lerpprog * r2 + (1 - lerpprog) * r1;
		uint8_t lg = lerpprog * g2 + (1 - lerpprog) * g1;
		uint8_t lb = lerpprog * b2 + (1 - lerpprog) * b1;
		ppu->buffer[i] = 0xff000000 | (lr << 16) | (lg << 8) | lb;
	}

	host_trade_buffer(&ppu->buffer);

	return true; // true if vblank
}

void ppu_exit(PPU *ppu) {
	free(ppu->buffer);
}
