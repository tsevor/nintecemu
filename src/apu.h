#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct APU {

} APU;

void apu_init(APU *apu);

void apu_clock(APU *apu, int cycles);
