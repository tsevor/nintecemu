#include "ppu.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "host.h"

#define NES_WIDTH 256
#define NES_HEIGHT 240

uint32_t palette[64] = {
	0xff747474, 0xff24188c, 0xff0000a8, 0xff44009c,
	0xff8c0074, 0xffa80010, 0xffa40000, 0xff7c0800,
	0xff402c00, 0xff004400, 0xff005000, 0xff003c14,
	0xff183c5c, 0xff000000, 0xff000000, 0xff000000,
	0xffbcbcbc, 0xff0070ec, 0xff2038ec, 0xff8000f0,
	0xffbc00bc, 0xffe40058, 0xffd82800, 0xffc84c0c,
	0xff887000, 0xff009400, 0xff00a800, 0xff009038,
	0xff008088, 0xff000000, 0xff000000, 0xff000000,
	0xfffcfcfc, 0xff3cbcfc, 0xff5c94fc, 0xffcc88fc,
	0xfff478fc, 0xfffc74b4, 0xfffc7460, 0xfffc9838,
	0xfff0bc3c, 0xff80d010, 0xff4cdc48, 0xff58f898,
	0xff00e8d8, 0xff787878, 0xff000000, 0xff000000,
	0xfffcfcfc, 0xffa8e4fc, 0xffc4d4fc, 0xffd4c8fc,
	0xfffcc4fc, 0xfffcc4d8, 0xfffcbcb0, 0xfffcd8a8,
	0xfffce4a0, 0xffe0fca0, 0xffa8f0bc, 0xffb0fccc,
	0xff9cfcf0, 0xffc4c4c4, 0xff000000, 0xff000000
};

void ppu_init(PPU *ppu) {
	for (int i = 0; i < 0x800; i++)
		ppu->vram[i] = 0;
	for (int i = 0; i < 0x20; i++)
		ppu->pram[i] = 0;
	for (int i = 0; i < 0x100; i++)
		ppu->oam.raw[i] = 0;

	ppu->scanline = 0;
	ppu->dot = 0;
}

// helpers
uint8_t ppu_use_buffer(PPU *ppu, uint8_t val) {
	uint8_t temp = ppu->buffer;
	ppu->buffer = val;
	return temp;
}

uint8_t ppu_read_nt(PPU *ppu, uint16_t addr) {
	uint16_t nt_index = cart_mirror_nt(ppu->cart, (addr >> 10) & 0x03);
	return ppu->vram[nt_index << 10 | (addr & 0x3ff)];
}

void ppu_write_nt(PPU *ppu, uint16_t addr, uint8_t val) {
	uint16_t nt_index = cart_mirror_nt(ppu->cart, (addr >> 10) & 0x03);
	ppu->vram[nt_index << 10 | (addr & 0x3ff)] = val;
}

// main bus read registers
uint8_t ppu_read_stat(PPU *ppu) {
	uint8_t val = ppu->stat.raw;
	ppu->stat.V = 0;
	ppu->w = false;
	return val;
}

uint8_t ppu_read_oamdata(PPU *ppu) {
	return ppu->oam.raw[ppu->oamaddr];
}

// main bus write registers
void ppu_write_ctrl(PPU *ppu, uint8_t val) {
	ppu->ctrl.raw = val;
	ppu->t.nametable_x = ppu->ctrl.N & 0x01;
	ppu->t.nametable_y = (ppu->ctrl.N >> 1) & 0x01;
}

void ppu_write_mask(PPU *ppu, uint8_t val) {
	ppu->mask.raw = val;
}

void ppu_write_oamaddr(PPU *ppu, uint8_t val) {
	ppu->oamaddr = val;
}

void ppu_write_oamdata(PPU *ppu, uint8_t val) {
	ppu->oam.raw[ppu->oamaddr] = val;
	ppu->oamaddr++;
}

void ppu_write_scrl(PPU *ppu, uint8_t val) {
	if (!ppu->w) {
		ppu->t.coarse_x = val >> 3;
		ppu->x = val & 0x07;
	} else {
		ppu->t.fine_y = val & 0x07;
		ppu->t.coarse_y = val >> 3;
	}
	ppu->w = !ppu->w;
}

void ppu_write_addr(PPU *ppu, uint8_t val) {
	if (!ppu->w) {
		ppu->t.raw = (ppu->t.raw & 0x80ff) | ((val & 0x3f) << 8);
	} else {
		ppu->t.raw = (ppu->t.raw & 0xff00) | val;
		ppu->v.raw = ppu->t.raw;
	}
	ppu->w = !ppu->w;
}

void ppu_oam_dma(PPU *ppu, uint8_t *data) {
	uint8_t addr = ppu->oamaddr;
	for (int i = 0; i < 256; i++) {
		ppu->oam.raw[addr] = data[i];
		addr++;
	}
	ppu->oamaddr = addr;
}

// ppu bus read/write
uint8_t ppu_read(PPU *ppu) {
	uint16_t a = ppu->v.raw & 0x3fff;
	if (a >= 0x3000 && a <= 0x3eff) a -= 0x1000;

	uint8_t result = 0;

	switch (a) {
	case 0x0000 ... 0x1fff:
		result = ppu_use_buffer(ppu, cart_read_chr(ppu->cart, a));
		break;
	case 0x2000 ... 0x2fff:
		result = ppu_use_buffer(ppu, ppu_read_nt(ppu, a));
		break;
	case 0x3f10:
	case 0x3f14:
	case 0x3f18:
	case 0x3f1c:
		result = ppu->pram[(a ^ 0x10) & 0x1f];
		ppu->buffer = ppu_read_nt(ppu, a - 0x1000);
		break;
	case 0x3f00 ... 0x3f0f:
	case 0x3f11 ... 0x3f13:
	case 0x3f15 ... 0x3f17:
	case 0x3f19 ... 0x3f1b:
	case 0x3f1d ... 0x3fff:
		result = ppu->pram[a & 0x1f];
		ppu->buffer = ppu_read_nt(ppu, a - 0x1000);
		break;
	default:
		result = ppu->buffer;
		break;
	}

	ppu->v.raw += (ppu->ctrl.I == 0) ? 1 : 32;
	return result;
}


void ppu_write(PPU *ppu, uint8_t val) {
	uint16_t a = ppu->v.raw & 0x3fff;
	if (a >= 0x3000 && a <= 0x3eff) a -= 0x1000;

	switch (a) {
		case 0x0000 ... 0x1fff:
			cart_write_chr(ppu->cart, a, val);
			break;
		case 0x2000 ... 0x2fff:
			ppu_write_nt(ppu, a, val);
			break;
		case 0x3f10:
		case 0x3f14:
		case 0x3f18:
		case 0x3f1c:
			ppu->pram[(a ^ 0x10) & 0x1f] = val;
			break;
		case 0x3f00 ... 0x3f0f:
		case 0x3f11 ... 0x3f13:
		case 0x3f15 ... 0x3f17:
		case 0x3f19 ... 0x3f1b:
		case 0x3f1d ... 0x3fff:
			ppu->pram[a & 0x1f] = val;
			break;
		default:
			break;
	}

	ppu->v.raw += (ppu->ctrl.I == 0) ? 1 : 32;
}

void ppu_draw_dot(PPU *ppu) {
	if (ppu->scanline < 240 || ppu->scanline >= 240) return;
	if (ppu->dot < 1 || ppu->dot > 256) return;
	uint8_t x = ppu->dot - 1;
	uint8_t y = ppu->scanline;

	(void)x;
	(void)y;
}


bool ppu_step(PPU *ppu) {
	bool vblank_start = false;
	ppu->dot++;

	if (ppu->dot == 341) {
		ppu->scanline++;
		ppu->dot = 0;
	}

	if (ppu->scanline == 262) {
		ppu->scanline = 0;
	}

	if (ppu->scanline == 241 && ppu->dot == 1) {
		ldeb2("ppu_step: vblank triggered (nmi enable: %d)\n", ppu->ctrl.V);
		ppu->stat.V = 1;
		vblank_start = true;
		host_swap_buffer();
	}

	if (ppu->scanline == 261 && ppu->dot == 1) {
		ppu->stat.V = 0;
		ppu->stat.S = 0;
		ppu->stat.O = 0;
	}

	if (ppu->mask.b || ppu->mask.s) {

		if (ppu->scanline < 240 || ppu->scanline == 261) {
			if (ppu->dot == 257) {
				ppu->v.nametable_x = ppu->t.nametable_x;
				ppu->v.coarse_x = ppu->t.coarse_x;
			}
		}

		if (ppu->scanline == 261) {
			if (ppu->dot >= 280 && ppu->dot <= 304) {
				ppu->v.nametable_y = ppu->t.nametable_y;
				ppu->v.coarse_y = ppu->t.coarse_y;
				ppu->v.fine_y = ppu->t.fine_y;
			}
		}
	}

	ppu_draw_dot(ppu);

	return vblank_start;
}

bool ppu_clock(PPU *ppu, int cycles) {
	bool vblank = false;
	for (int i = 0; i < cycles; i++) {
		if (ppu_step(ppu))
			vblank = true;
	}
	return vblank;
}
