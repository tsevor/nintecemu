#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct MapperInterface MapperInterface;

typedef struct MapperInterface {
	uint8_t (*read_prg)(MapperInterface *mi, uint16_t addr);
	void (*write_prg)(MapperInterface *mi, uint16_t addr, uint8_t val);
	uint8_t (*read_chr)(MapperInterface *mi, uint16_t addr);
	void (*write_chr)(MapperInterface *mi, uint16_t addr, uint8_t val);

	void (*reset)(MapperInterface *mi);

	uint8_t (*mirror_nt)(MapperInterface *mi, int nt_idx);

	bool (*irq_active)(MapperInterface *mi);
	void (*scanline)(MapperInterface *mi);

	void (*free)(MapperInterface *mi);

	void *mapper_struct;
} MapperInterface;
