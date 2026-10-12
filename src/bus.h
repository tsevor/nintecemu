#pragma once

#include <stdint.h>
#include <stddef.h>

#include "cart.h"
#include "apu.h"
#include "ppu.h"

typedef struct CPU CPU;

typedef struct Bus {
	uint8_t memory[0x800];

	uint8_t open_bus;

	// connected devices

	Cart *cart;
	CPU *cpu;
	PPU *ppu;
	APU *apu;
} Bus;

void bus_init(Bus *bus);

bool bus_clock(Bus *bus, int cycles);

uint8_t bus_read(Bus *bus, uint16_t addr);
void bus_write(Bus *bus, uint16_t addr, uint8_t val);
