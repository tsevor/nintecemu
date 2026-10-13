#include "cart.h"

#include <stdio.h>
#include <stdlib.h>

int cart_load(Cart *cart, const char *filename) {
	linfo("load_cart: filename=\"%s\"\n", filename);

	if (!cart) {
		lerror("load_cart: passed unallocated cart\n");
		return 1;
	}

	FILE *file = fopen(filename, "rb");

	if (!file) {
		lerror("load_cart: file doesn't exist\n");
		return 1;
	}

	size_t read;

	read = fread(cart->header, 1, 16, file);
	if (read != 16) {
		lerror("load_cart: couldn't read file header\n");
		fclose(file);
		return 1;
	}

	for (int i = 0; i < 4; i++)
		if (cart->header[i] != "NES\x1a"[i]) {
			lerror("load_cart: file header lacks magic bytes NES\\x1a\n");
			fclose(file);
			return 1;
		}

	cart->prg_banks_count = cart->header[4];
	cart->chr_banks_count = cart->header[5];

	cart->prg_banks_size = cart->prg_banks_count * PRG_BANK_SIZE;
	cart->chr_banks_size = cart->chr_banks_count * CHR_BANK_SIZE;

	uint8_t flags6 = cart->header[6];
	uint8_t flags7 = cart->header[7];

	cart->mapper = (flags7 & 0xf0) | (flags6 & 0xf0) >> 4;

	// temporary until mappers are properly implemented
	if (cart->mapper != 0) {
		lerror("load_cart: mapper not supported: %d", cart->mapper);
		fclose(file);
		return 1;
	}

	cart->four_screen = (flags6 & 0x08) != 0;
	cart->has_trainer = (flags6 & 0x04) != 0;
	cart->battery_backed = (flags6 & 0x02) != 0;
	cart->vertical_mirroring = (flags6 & 0x01) != 0;

	if (cart->four_screen)
		cart->mirroring = MIRROR_FOUR_SCREEN;
	else if (cart->vertical_mirroring)
		cart->mirroring = MIRROR_VERTICAL;
	else
		cart->mirroring = MIRROR_HORIZONTAL;

	cart->ines_version = (flags7 & 0x0c) >> 2;

	if (cart->ines_version == 2) {
		lwarn("load_cart: ines 2.0 not fully supported\n");
	}

	if (cart->has_trainer)
		fseek(file, 512, SEEK_CUR);

	cart->prg_banks = malloc(cart->prg_banks_size);
	read = fread(cart->prg_banks, 1, cart->prg_banks_size, file);

	if (read != cart->prg_banks_size){
		lerror("load_cart: couldn't read prg banks\n");
		free(cart->prg_banks);
		fclose(file);
		return 1;
	}

	if (cart->chr_banks_count) {
		cart->chr_banks = malloc(cart->chr_banks_size);
		read = fread(cart->chr_banks, 1, cart->chr_banks_size, file);

		if (read != cart->chr_banks_size){
			lerror("load_cart: couldn't read chr banks\n");
			free(cart->prg_banks);
			free(cart->chr_banks);
			fclose(file);
			return 1;
		}
	} else {
		// chr-ram if count is 0
		cart->chr_banks = malloc(CHR_BANK_SIZE);
	}

	fclose(file);

	linfo("load_cart: completed\n");

	return 0;
}


// todo: replace these to be dynamic with mappers
uint8_t cart_read_prg(Cart *cart, uint16_t addr) {
	return cart->prg_banks[cart->prg_banks_count == 1 ? (addr - 0x8000) & 0x3fff : addr - 0x8000];
}

void cart_write_prg(Cart *cart, uint16_t addr, uint8_t val) {
	(void)cart;
	(void)addr;
	(void)val;
	// cart->prg_banks[addr - 0x8000] = val;
}

uint8_t cart_read_chr(Cart *cart, uint16_t addr) {
	return cart->chr_banks[addr];
}

void cart_write_chr(Cart *cart, uint16_t addr, uint8_t val) {
	(void)cart;
	(void)addr;
	(void)val;
	// can't
}

uint8_t cart_mirror_nt(Cart *cart, int nt_idx) {
	int lookup[3][4] = {
		{0, 0, 2, 2}, // HORIZONTAL
		{0, 1, 0, 1}, // VERTICAL
		{0, 1, 2, 3}  // FOUR_SCREEN
	};
	return lookup[cart->mirroring][nt_idx];
}
