#pragma once

#include <stdint.h>
#include <stddef.h>

#include "log.h"
#include "bus.h"
#include "host.h"


typedef struct CPU {
	// registers

	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t sp;
	uint16_t pc;
	union {
		uint8_t p;
		struct {
			uint8_t c : 1;
			uint8_t z : 1;
			uint8_t i : 1;
			uint8_t d : 1;
			uint8_t b : 1;
			uint8_t u : 1;
			uint8_t v : 1;
			uint8_t n : 1;
		};
	};

	// other data

	uint8_t jammed;
	uint64_t cycles;
	uint64_t stall;

	// connected components
	Bus *bus;

} CPU;

void cpu_poweron(CPU *cpu);
void cpu_reset(CPU *cpu);

void cpu_step(CPU *cpu);
void cpu_run(CPU *cpu);

void cpu_nmi(CPU *cpu);
