#pragma once

#include <stdint.h>
#include <stddef.h>

#include "cart.h"

typedef struct PPU {
	Cart *cart;
} PPU;

void ppu_init(PPU *apu);

bool ppu_clock(PPU *ppu, int cycles);
