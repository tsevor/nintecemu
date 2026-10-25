#include <stdlib.h>
#include <stdint.h>

#include "mapper.h"
#include "mappers/NROM.h"


typedef struct MapperNROM {
	Cart *cart;
} MapperNROM;

#define mstruct ((MapperNROM*)mi->mapper_struct)

uint8_t m_NROM_read_prg(MapperInterface *mi, uint16_t addr) {
	if (addr < 0x8000) return 0; // TODO: pass open bus somehow
	addr = mstruct->cart->prg_banks_count == 1
		? addr & 0x3fff : addr & 0x7fff;
	return mstruct->cart->prg_banks[addr];
}

void m_NROM_write_prg(MapperInterface *mi, uint16_t addr, uint8_t val) {
	(void)mi;
	(void)addr;
	(void)val;
}

uint8_t m_NROM_read_chr(MapperInterface *mi, uint16_t addr) {
	if (addr < 0x2000)
		return mstruct->cart->chr_banks[addr];
	return 0;
}

void m_NROM_write_chr(MapperInterface *mi, uint16_t addr, uint8_t val) {
	// if banks == 0, then chr ram
	if (mstruct->cart->chr_banks_count == 0 && addr < 0x2000)
		mstruct->cart->chr_banks[addr] = val;
}

uint8_t m_NROM_mirror_nt(MapperInterface *mi, int nt_idx) {
	static const uint8_t lut[3][4] = {
		// nametable            0  1  2  3
		[MIRROR_HORIZONTAL]  = {0, 0, 2, 2},
		[MIRROR_VERTICAL]    = {0, 1, 0, 1},
		[MIRROR_FOUR_SCREEN] = {0, 1, 2, 3}
	};
	return lut[mstruct->cart->mirroring][nt_idx];
}

void m_NROM_reset(MapperInterface *mi) {
	(void)mi;
}

void m_NROM_free(MapperInterface *mi) {
	free(mstruct);
}

void m_NROM_init(MapperInterface *mi, Cart *cart) {
	mi->mapper_struct = malloc(sizeof(MapperNROM));
	mstruct->cart = cart;

	mi->read_prg = m_NROM_read_prg;
	mi->write_prg = m_NROM_write_prg;
	mi->read_chr = m_NROM_read_chr;
	mi->write_chr = m_NROM_write_chr;
	mi->mirror_nt = m_NROM_mirror_nt;

	mi->reset = m_NROM_reset;
	mi->free = m_NROM_free;
}
