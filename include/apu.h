#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "bus.h"

typedef struct APU {
	Bus *bus;
} APU;

void apu_init(APU *apu);

void apu_reset(APU *apu);

void apu_clock(APU *apu, int cycles);

uint8_t apu_read(APU *apu, uint16_t addr);
void apu_write(APU *apu, uint16_t addr, uint8_t val);

bool apu_irq_active(APU *apu);
