#pragma once

#include <stdint.h>
#include <stddef.h>

#include "cart.h"
#include "apu.h"
#include "ppu.h"
#include "controller.h"

typedef struct CPU CPU;

typedef struct Bus {
	uint8_t memory[0x800];

	uint8_t open_bus;
	uint8_t ppu_open_bus;


	// cpu fetch state
	bool vblank_started;

	// connected devices

	Cart *cart;
	CPU *cpu;
	PPU *ppu;
	APU *apu;

	Controller *controller1;
	Controller *controller2;
} Bus;

void bus_init(Bus *bus);

void bus_reset(Bus *bus);

void bus_clock(Bus *bus);

uint8_t bus_read(Bus *bus, uint16_t addr);
void bus_write(Bus *bus, uint16_t addr, uint8_t val);

#include "cpu.h"
