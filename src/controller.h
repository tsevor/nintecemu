#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct Controller {
	union {
		uint8_t raw;
		struct {
			uint8_t a : 1;
			uint8_t b : 1;
			uint8_t select : 1;
			uint8_t start : 1;
			uint8_t up : 1;
			uint8_t down : 1;
			uint8_t left : 1;
			uint8_t right : 1;
		};
	} state;
	bool strobe;
	int read_idx;
} Controller;

uint8_t controller_read(Controller *ctrl);
void controller_write(Controller *ctrl, uint8_t val);
