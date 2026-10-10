#pragma once

#include <stdint.h>

#include "cart.h"

typedef struct Bus {
	uint8_t memory[0x800];

	uint8_t open_bus;

	// connected devices

	Cart *cart;
} Bus;

void bus_init(Bus *bus);

uint8_t bus_read(Bus *bus, uint16_t addr);
void bus_write(Bus *bus, uint16_t addr, uint8_t val);
