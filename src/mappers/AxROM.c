#include <stdlib.h>
#include <stdint.h>

#include "cart.h"
#include "mapper.h"
#include "mappers/AxROM.h"




typedef struct MapperAxROM {
	Cart *cart;

	uint8_t mirror_page;
	uint32_t prg_offset;
} MapperAxROM;

#define mstruct ((MapperAxROM*)mi->mapper_struct)

uint8_t m_AxROM_read_prg(MapperInterface *mi, uint16_t addr) {
	if (addr < 0x8000) return 0; // TODO: pass open bus somehow
	return mstruct->cart->prg_banks[mstruct->prg_offset + (addr & 0x7fff)];
}

void m_AxROM_write_prg(MapperInterface *mi, uint16_t addr, uint8_t val) {
	if (addr < 0x8000) return;
	uint8_t prg_bank = val & 0x0f;
	mstruct->prg_offset = (prg_bank << 15) & ((mstruct->cart->prg_banks_count << 14) - 1);
	mstruct->mirror_page = (val & 0x10) >> 4;
}

uint8_t m_AxROM_read_chr(MapperInterface *mi, uint16_t addr) {
	if (addr < 0x2000)
		return mstruct->cart->chr_banks[addr];
	return 0;
}

void m_AxROM_write_chr(MapperInterface *mi, uint16_t addr, uint8_t val) {
	// if banks == 0, then chr ram
	if (mstruct->cart->chr_banks_count == 0 && addr < 0x2000)
		mstruct->cart->chr_banks[addr] = val;
}

uint8_t m_AxROM_mirror_nt(MapperInterface *mi, int nt_idx) {
	(void)nt_idx;
	return mstruct->mirror_page;
}

void m_AxROM_reset(MapperInterface *mi) {
	mstruct->mirror_page = 0;
	mstruct->prg_offset = 0;
}

void m_AxROM_free(MapperInterface *mi) {
	free(mstruct);
}

void m_AxROM_init(MapperInterface *mi, Cart *cart) {
	mi->mapper_struct = malloc(sizeof(MapperAxROM));
	mstruct->cart = cart;

	mi->read_prg = m_AxROM_read_prg;
	mi->write_prg = m_AxROM_write_prg;
	mi->read_chr = m_AxROM_read_chr;
	mi->write_chr = m_AxROM_write_chr;

	mi->mirror_nt = m_AxROM_mirror_nt;

	mi->reset = m_AxROM_reset;
	mi->free = m_AxROM_free;

	mstruct->mirror_page = 0;
	mstruct->prg_offset = 0;
}
