#pragma once

#include <stdint.h>
#include <stddef.h>

#include "host.h"
#include "log.h"
#include "mapper.h"

#define PRG_BANK_SIZE 0x4000
#define CHR_BANK_SIZE 0x2000

#define TRAINER_SIZE 0x200

typedef enum MirrorType {
	MIRROR_HORIZONTAL,
	MIRROR_VERTICAL,
	MIRROR_FOUR_SCREEN
} MirrorType;

typedef struct Cart {
	uint8_t header[16];
	int ines_version;

	size_t prg_banks_count;
	size_t chr_banks_count;
	size_t prg_banks_size;
	size_t chr_banks_size;

	uint8_t *prg_banks;
	uint8_t *chr_banks;

	int mapperid;

	bool four_screen;
	bool has_trainer;
	bool battery_backed;
	bool vertical_mirroring;

	MirrorType mirroring;

	int trainer_size;

	MapperInterface mapper;

} Cart;

int cart_load(Cart *cart, const char *filename);
void cart_free(Cart *cart);

uint8_t cart_read_prg(Cart *cart, uint16_t addr);
void cart_write_prg(Cart *cart, uint16_t addr, uint8_t val);

uint8_t cart_read_chr(Cart *cart, uint16_t addr);
void cart_write_chr(Cart *cart, uint16_t addr, uint8_t val);

uint8_t cart_mirror_nt(Cart *cart, int nt_idx);

void cart_reset(Cart *cart);
bool cart_irq_active(Cart *cart);
void cart_scanline(Cart *cart);

// include all mappers here

#include "mappers/NROM.h" // 0
#include "mappers/AxROM.h" // 7
