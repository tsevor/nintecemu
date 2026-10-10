#pragma once

#include <stdint.h>

#include "log.h"
#include "bus.h"


typedef enum AddrMode {
	IMP, // implicit
	ACC, // accumulator
	IMM, // immediate
	ZPG, // zeropage
	ZPX, // zeropage,X
	ZPY, // zeropage,Y
	REL, // relative
	ABS, // absolute
	ABX, // absolute,X
	ABY, // absolute,Y
	IND, // indirect
	INX, // (indirect,X)
	INY, // (indirect),Y
} AddrMode;

typedef enum AccessType {
	READ,
	WRITE,
	RMW
} AccessType;

typedef struct CPU {
	// registers

	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t sp;
	uint16_t pc;
	union {
		uint8_t p;
		struct {
			uint8_t c : 1;
			uint8_t z : 1;
			uint8_t i : 1;
			uint8_t d : 1;
			uint8_t b : 1;
			uint8_t u : 1;
			uint8_t v : 1;
			uint8_t n : 1;
		};
	};

	// other data

	uint8_t jammed;
	uint64_t cycles;

	// connected components
	Bus *bus;

} CPU;

void cpu_poweron(CPU *cpu);
void cpu_reset(CPU *cpu);

void cpu_step(CPU *cpu);
void cpu_run(CPU *cpu);

uint8_t cpu_read(CPU *cpu, uint16_t addr);
void cpu_write(CPU *cpu, uint16_t addr, uint8_t val);
uint16_t cpu_read16(CPU *cpu, uint16_t addr);
void cpu_write16(CPU *cpu, uint16_t addr, uint16_t val); // probably never used ever

uint8_t cpu_pull(CPU *cpu);
void cpu_push(CPU *cpu, uint8_t val);
uint16_t cpu_pull16(CPU *cpu);
void cpu_push16(CPU *cpu, uint16_t val);

void cpu_set_zn(CPU *cpu, uint8_t val);
void cpu_branch(CPU *cpu, AddrMode mode, int condition);

uint16_t cpu_oper_addr(CPU *cpu, AddrMode mode, AccessType type);

void cpu_op_adc(CPU *cpu, AddrMode mode);
void cpu_op_and(CPU *cpu, AddrMode mode);
void cpu_op_asl(CPU *cpu, AddrMode mode);
void cpu_op_bcc(CPU *cpu, AddrMode mode);
void cpu_op_bcs(CPU *cpu, AddrMode mode);
void cpu_op_beq(CPU *cpu, AddrMode mode);
void cpu_op_bit(CPU *cpu, AddrMode mode);
void cpu_op_bmi(CPU *cpu, AddrMode mode);
void cpu_op_bne(CPU *cpu, AddrMode mode);
void cpu_op_bpl(CPU *cpu, AddrMode mode);
void cpu_op_brk(CPU *cpu, AddrMode mode);
void cpu_op_bvc(CPU *cpu, AddrMode mode);
void cpu_op_bvs(CPU *cpu, AddrMode mode);
void cpu_op_clc(CPU *cpu, AddrMode mode);
void cpu_op_cld(CPU *cpu, AddrMode mode);
void cpu_op_cli(CPU *cpu, AddrMode mode);
void cpu_op_clv(CPU *cpu, AddrMode mode);
void cpu_op_cmp(CPU *cpu, AddrMode mode);
void cpu_op_cpx(CPU *cpu, AddrMode mode);
void cpu_op_cpy(CPU *cpu, AddrMode mode);
void cpu_op_dec(CPU *cpu, AddrMode mode);
void cpu_op_dex(CPU *cpu, AddrMode mode);
void cpu_op_dey(CPU *cpu, AddrMode mode);
void cpu_op_eor(CPU *cpu, AddrMode mode);
void cpu_op_inc(CPU *cpu, AddrMode mode);
void cpu_op_inx(CPU *cpu, AddrMode mode);
void cpu_op_iny(CPU *cpu, AddrMode mode);
void cpu_op_jmp(CPU *cpu, AddrMode mode);
void cpu_op_jsr(CPU *cpu, AddrMode mode);
void cpu_op_lda(CPU *cpu, AddrMode mode);
void cpu_op_ldx(CPU *cpu, AddrMode mode);
void cpu_op_ldy(CPU *cpu, AddrMode mode);
void cpu_op_lsr(CPU *cpu, AddrMode mode);
void cpu_op_nop(CPU *cpu, AddrMode mode);
void cpu_op_ora(CPU *cpu, AddrMode mode);
void cpu_op_pha(CPU *cpu, AddrMode mode);
void cpu_op_php(CPU *cpu, AddrMode mode);
void cpu_op_pla(CPU *cpu, AddrMode mode);
void cpu_op_plp(CPU *cpu, AddrMode mode);
void cpu_op_rol(CPU *cpu, AddrMode mode);
void cpu_op_ror(CPU *cpu, AddrMode mode);
void cpu_op_rti(CPU *cpu, AddrMode mode);
void cpu_op_rts(CPU *cpu, AddrMode mode);
void cpu_op_sbc(CPU *cpu, AddrMode mode);
void cpu_op_sec(CPU *cpu, AddrMode mode);
void cpu_op_sed(CPU *cpu, AddrMode mode);
void cpu_op_sei(CPU *cpu, AddrMode mode);
void cpu_op_sta(CPU *cpu, AddrMode mode);
void cpu_op_stx(CPU *cpu, AddrMode mode);
void cpu_op_sty(CPU *cpu, AddrMode mode);
void cpu_op_tax(CPU *cpu, AddrMode mode);
void cpu_op_tay(CPU *cpu, AddrMode mode);
void cpu_op_tsx(CPU *cpu, AddrMode mode);
void cpu_op_txa(CPU *cpu, AddrMode mode);
void cpu_op_txs(CPU *cpu, AddrMode mode);
void cpu_op_tya(CPU *cpu, AddrMode mode);

void cpu_op_switch(CPU *cpu, uint8_t opcode);
