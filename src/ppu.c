#include "ppu.h"

#include <stdbool.h>
#include <stdint.h>
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
	for (int i = 0; i < 0x1000; i++)
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

    ppu->v.raw = (ppu->v.raw + (ppu->ctrl.I ? 32 : 1)) & 0x3fff;
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

    ppu->v.raw = (ppu->v.raw + (ppu->ctrl.I ? 32 : 1)) & 0x3fff;
}

void ppu_draw_dot(PPU *ppu) {
	// going for readability here
	// leaning heavy on compiler optimizations

	if (ppu->scanline < 0 || ppu->scanline >= 240) return;
	if (ppu->dot < 1 || ppu->dot > 256) return;
	uint8_t x = ppu->dot - 1;
	uint8_t y = ppu->scanline;

	uint8_t bg_palette = 0;
	uint16_t bg_pixel = 0;

	// background
	if (ppu->mask.b && (x >= 8 || ppu->mask.m)) {
		// get tile id
		uint16_t scroll_x = (ppu->v.coarse_x << 3) | ppu->x;
		uint16_t scroll_y = (ppu->v.coarse_y << 3) | ppu->v.fine_y;

		uint16_t eff_x = scroll_x + x;
		uint16_t eff_y = scroll_y + y;

		uint16_t nt_x = ppu->v.nametable_x ^ (eff_x >= 256);
		uint16_t nt_y = ppu->v.nametable_y ^ (eff_y >= 240);

		eff_x &= 0xff;
		if (eff_y >= 240) eff_y -= 240;

		uint8_t tile_x = eff_x >> 3;
		uint8_t tile_y = eff_y >> 3;

		uint16_t nt_base = 0x2000 | (nt_y << 11) | (nt_x << 10);
		uint16_t nt_addr = nt_base | (tile_y << 5) | tile_x;

		uint8_t tile_id = ppu_read_nt(ppu, nt_addr);

		// get palette
		uint8_t attr_group_x = tile_x >> 2;
		uint8_t attr_group_y = tile_y >> 2;
		uint16_t attr_addr = nt_base | 0x03c0 | (attr_group_y << 3) | attr_group_x;
		uint8_t attr_byte = ppu_read_nt(ppu, attr_addr);

		uint8_t pal_group_x = (tile_x >> 1) & 1;
		uint8_t pal_group_y = (tile_y >> 1) & 1;
		uint8_t shamt = (pal_group_y << 2) | (pal_group_x << 1);

		bg_palette = (attr_byte >> shamt) & 0x03;

		// get chr-rom data
		uint16_t rom_addr = (ppu->ctrl.B << 12) | (tile_id << 4) | (eff_y & 7);
		uint16_t tile_low = cart_read_chr(ppu->cart, rom_addr);
		uint16_t tile_high = cart_read_chr(ppu->cart, rom_addr + 8);
		shamt = 7 - (eff_x & 7);
		bg_pixel = (((tile_high >> shamt) & 1) << 1) | ((tile_low >> shamt) & 1);
	}

	uint8_t spr_pixel = 0;
	uint8_t spr_palette = 0;
	bool spr_priority = false;
	bool sprite_zero = false;

	// sprites
	if (ppu->mask.s && (x >= 8 || ppu->mask.M)) {
		int sprite_height = ppu->ctrl.H ? 16 : 8;

		for (int i = 0; i < 64; i++) {
			uint8_t sy = ppu->oam.sprites[i].y;
			uint8_t tile = ppu->oam.sprites[i].tileIdx;
			uint8_t attr = ppu->oam.sprites[i].attr;
			uint8_t sx = ppu->oam.sprites[i].x;

			sy = sy + 1; // 1 scanline delay
			if (sy == 0 || sy >= 240) continue;

			int diff_x = x - sx;
			int diff_y = y - sy;

			if (diff_x < 0 || diff_x >= 8) continue;
			if (diff_y < 0 || diff_y >= sprite_height) continue;

			if (attr & 0x80) // vertical flip
				diff_y = (sprite_height - 1) - diff_y;

			if (attr & 0x40) // horizontal flip
				diff_x = 7 - diff_x;

			uint16_t pattern_addr;
			if (sprite_height == 8) { // 8x8 sprites
				pattern_addr = (ppu->ctrl.S << 12) | (tile << 4) | diff_y;
			} else { // 8x16 sprites
				uint16_t bank = (tile & 1) << 12;
				uint8_t real_tile = (tile & 0xfe) | (diff_y >= 8);
				pattern_addr = bank | (real_tile << 4) | (diff_y & 7);
			}
			uint8_t pattern_low = cart_read_chr(ppu->cart, pattern_addr);
			uint8_t pattern_high = cart_read_chr(ppu->cart, pattern_addr + 8);
			uint8_t shamt = 7 - diff_x;
			spr_pixel = (((pattern_high >> shamt) & 1) << 1) | ((pattern_low >> shamt) & 1);

			if (spr_pixel != 0) {
				spr_palette = (attr & 0x03) + 4; // palettes 4-7
				spr_priority = (attr & 0x20) == 0;
				if (i == 0) sprite_zero = true;
				break;
			}
		}
	}

	uint8_t final_pixel = 0;
	uint8_t final_palette = 0;

	if (!spr_pixel) { // only bg (or neither)
		final_pixel = bg_pixel;
		final_palette = bg_palette;
	} else if (bg_pixel) { // both
		if (sprite_zero && x != 255)
			ppu->stat.S = 1;
		if (spr_priority) {
			final_pixel = spr_pixel;
			final_palette = spr_palette;
		} else {
			final_pixel = bg_pixel;
			final_palette = bg_palette;
		}
	} else { // only sprite
		final_pixel = spr_pixel;
		final_palette = spr_palette;
	}

	uint16_t palette_addr = 0;
	if (final_pixel != 0) {
		palette_addr = (final_palette << 2) | final_pixel;
	}
	uint8_t palette_idx = ppu->pram[palette_addr & 0x1f];
	uint32_t final_color = 0xff000000 | palette[palette_idx & 0x3f];

	host_screen_buffer[y * 256 + x] = final_color;
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
