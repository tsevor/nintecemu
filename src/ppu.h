#pragma once

#include <stdint.h>
#include <stddef.h>

#include "cart.h"

typedef union PPULoopyRegister {
	uint16_t raw;
	struct {
		uint16_t coarse_x    : 5; // Bits 0-4
		uint16_t coarse_y    : 5; // Bits 5-9
		uint16_t nametable_x : 1; // Bit 10
		uint16_t nametable_y : 1; // Bit 11
		uint16_t fine_y      : 3; // Bits 12-14
		uint16_t unused      : 1; // Bit 15
	};
} PPULoopyRegister;

typedef struct PPU {
	// registers
	// in these bit structs, lsb highest
	union {
		uint8_t raw;
		struct {
			uint8_t N : 2;
			uint8_t I : 1;
			uint8_t S : 1;
			uint8_t B : 1;
			uint8_t H : 1;
			uint8_t P : 1;
			uint8_t V : 1;
		};
	} ctrl;

	union {
		uint8_t raw;
		struct {
			uint8_t G2 : 1;
			uint8_t m : 1;
			uint8_t M : 1;
			uint8_t b : 1;
			uint8_t s : 1;
			uint8_t R : 1;
			uint8_t G : 1;
			uint8_t B : 1;
		};
	} mask;

	union {
		uint8_t raw;
		struct {
			uint8_t unused : 5;
			uint8_t O : 1;
			uint8_t S : 1;
			uint8_t V : 1;
		};
	} stat;

	uint8_t oamaddr;
	uint8_t oamdata;

	PPULoopyRegister v;
	PPULoopyRegister t;
	uint8_t x : 3;
	bool w;

	uint8_t vram[0x1000]; // double real size for 4-screen
	uint8_t pram[0x20];

	union {
		uint8_t raw[0x100];
		struct {
			uint8_t y : 8;
			uint8_t tileIdx : 8;
			uint8_t attr : 8;
			uint8_t x : 8;
		} sprites[0x40];
	} oam;

	uint8_t buffer;

	// other workings
	int scanline;
	int dot;

	// connected parts
	Cart *cart;
} PPU;

void ppu_init(PPU *ppu);

bool ppu_clock(PPU *ppu, int cycles);
bool ppu_step(PPU *ppu);

// main bus read registers
uint8_t ppu_read_oamdata(PPU *ppu);
uint8_t ppu_read_stat(PPU *ppu);

// main bus write registers
void ppu_write_ctrl(PPU *ppu, uint8_t val);
void ppu_write_mask(PPU *ppu, uint8_t val);
void ppu_write_oamaddr(PPU *ppu, uint8_t val);
void ppu_write_oamdata(PPU *ppu, uint8_t val);
void ppu_write_scrl(PPU *ppu, uint8_t val);
void ppu_write_addr(PPU *ppu, uint8_t val);

void ppu_oam_dma(PPU *ppu, uint8_t *data);

// ppu bus read/write
uint8_t ppu_read(PPU *ppu);
void ppu_write(PPU *ppu, uint8_t val);
