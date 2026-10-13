#pragma once

#include <stdint.h>
#include <stddef.h>

#include "cart.h"


typedef struct PPU {
	int scanline;
	int dot;

	uint32_t *buffer;

	Cart *cart;
} PPU;

void ppu_init(PPU *ppu);

bool ppu_clock(PPU *ppu, int cycles);

void ppu_exit(PPU *ppu)
