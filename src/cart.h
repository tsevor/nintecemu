#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "log.h"

#define PRG_BANK_SIZE 0x4000
#define CHR_BANK_SIZE 0x2000

typedef struct Bus Bus;

typedef struct {
	uint8_t header[16];
	int ines_version;

	size_t prg_banks_count;
	size_t chr_banks_count;
	size_t prg_banks_size;
	size_t chr_banks_size;

	uint8_t *prg_banks;
	uint8_t *chr_banks;

	int mapper;

	int four_screen;
	int has_trainer;
	int battery_backed;
	int vertical_mirroring;

	int trainer_size;

} Cart;

int cart_load(Cart *cart, const char *filename);

uint8_t cart_read_prg(Cart *cart, uint16_t addr);
void cart_write_prg(Cart *cart, uint16_t addr, uint8_t val);

uint8_t cart_read_chr(Cart *cart, uint16_t addr);
void cart_write_chr(Cart *cart, uint16_t addr, uint8_t val);
