#include "cpu.h"

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


uint8_t cpu_read(CPU *cpu, uint16_t addr) {
	cpu->cycles++;
	return bus_read(cpu->bus, addr);
}

void cpu_write(CPU *cpu, uint16_t addr, uint8_t val) {
	cpu->cycles++;
	bus_write(cpu->bus, addr, val);
}

uint16_t cpu_read16(CPU *cpu, uint16_t addr) {
	return cpu_read(cpu, addr) | (cpu_read(cpu, addr + 1) << 8);
}

void cpu_write16(CPU *cpu, uint16_t addr, uint16_t val) {
	cpu_write(cpu, addr, val & 0xff);
	cpu_write(cpu, addr + 1, val >> 8);
}


uint8_t cpu_pull(CPU *cpu) {
	return cpu_read(cpu, 0x100 + (++cpu->sp));
}

void cpu_push(CPU *cpu, uint8_t val) {
	cpu_write(cpu, 0x100 + cpu->sp--, val);
}

uint16_t cpu_pull16(CPU *cpu) {
	return cpu_pull(cpu) | (cpu_pull(cpu) << 8);
}

void cpu_push16(CPU *cpu, uint16_t val) {
	cpu_push(cpu, val >> 8);
	cpu_push(cpu, val & 0xff);
}

uint16_t cpu_oper_addr(CPU *cpu, AddrMode mode, AccessType type) {
	uint8_t base;
	uint16_t base16;
	uint16_t addr;

	switch (mode) {
	case IMP:
	case ACC:
		cpu_read(cpu, cpu->pc);
		return 0;
	case IMM:
		return cpu->pc++;
	case ZPG:
		return cpu_read(cpu, cpu->pc++);
	case ZPX:
		base = cpu_read(cpu, cpu->pc++);
		cpu_read(cpu, base); // dummy read
		return (base + cpu->x) & 0xff;
	case ZPY:
		base = cpu_read(cpu, cpu->pc++);
		cpu_read(cpu, base); // dummy read
		return (base + cpu->y) & 0xff;
	case REL:
		base = cpu_read(cpu, cpu->pc++);
		return cpu->pc + (int8_t)base;
	case ABS:
		addr = cpu_read16(cpu, cpu->pc);
		cpu->pc += 2;
		return addr;
	case ABX:
		base16 = cpu_read16(cpu, cpu->pc);
		cpu->pc += 2;
		addr = base16 + cpu->x;
		if (type != READ || (base16 & 0xff00) != (addr & 0xff00))
			cpu_read(cpu, (base16 & 0xff00) | (addr & 0x00ff)); // dummy read
		return addr;
	case ABY:
		base16 = cpu_read16(cpu, cpu->pc);
		cpu->pc += 2;
		addr = base16 + cpu->y;
		if (type != READ || (base16 & 0xff00) != (addr & 0xff00))
			cpu_read(cpu, (base16 & 0xff00) | (addr & 0x00ff)); // dummy read
		return addr;
	case IND:
		base16 = cpu_read16(cpu, cpu->pc);
		cpu->pc += 2;
		if ((base16 & 0x00ff) == 0x00ff)
			addr = cpu_read(cpu, base16) | (cpu_read(cpu, base16 & 0xff00) << 8);
		else
			addr = cpu_read(cpu, base16) | (cpu_read(cpu, base16 + 1) << 8);
		return addr;
	case INX:
		base = cpu_read(cpu, cpu->pc++);
		cpu_read(cpu, base); // dummy read
		return cpu_read(cpu, (base + cpu->x) & 0xff) | (cpu_read(cpu, (base + cpu->x + 1) & 0xff) << 8);
	case INY:
		base = cpu_read(cpu, cpu->pc++);
		base16 = cpu_read(cpu, base) | (cpu_read(cpu, (base + 1) & 0xff) << 8);
		addr = base16 + cpu->y;
		if (type != READ || (base16 & 0xff00) != (addr & 0xff00)) {
			cpu_read(cpu, (base16 & 0xff00) | (addr & 0x00ff)); // dummy read
		}
		return addr;
	default:
		lerror("cpu_oper_addr: reached default case, mode: %d\n", mode);
		host_exit(1);
		return 0; // bypass compiler error
	}
}

void cpu_set_zn(CPU *cpu, uint8_t val) {
	cpu->z = val == 0;
	cpu->n = (val & 0x80) != 0;
}

void cpu_branch(CPU *cpu, AddrMode mode, int condition) {
	uint16_t target = cpu_oper_addr(cpu, mode, READ);
	if (condition) {
		cpu_read(cpu, cpu->pc);
		if ((cpu->pc & 0xFF00) != (target & 0xFF00))
			cpu_read(cpu, (cpu->pc & 0xFF00) | (target & 0x00FF));
		cpu->pc = target;
	}
}

// official opcodes

void cpu_op_adc(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	uint8_t val = cpu_read(cpu, addr);
	uint16_t temp = cpu->a + val + cpu->c;
	cpu->v = (~(cpu->a ^ val) & (cpu->a ^ temp) & 0x80) != 0;
	cpu->c = temp > 0xff;
	cpu->a = temp;
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_and(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	cpu->a &= cpu_read(cpu, addr);
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_asl(CPU *cpu, AddrMode mode) {
	if (mode == ACC) {
		cpu_oper_addr(cpu, mode, READ);
		cpu->c = (cpu->a & 0x80) != 0;
		cpu->a <<= 1;
		cpu_set_zn(cpu, cpu->a);
	} else {
		uint16_t addr = cpu_oper_addr(cpu, mode, RMW);
		uint8_t val = cpu_read(cpu, addr);
		cpu_write(cpu, addr, val);
		cpu->c = (val & 0x80) != 0;
		val <<= 1;
		cpu_write(cpu, addr, val);
		cpu_set_zn(cpu, val);
	}
}

void cpu_op_bcc(CPU *cpu, AddrMode mode) {
	cpu_branch(cpu, mode, !cpu->c);
}

void cpu_op_bcs(CPU *cpu, AddrMode mode) {
	cpu_branch(cpu, mode, cpu->c);
}

void cpu_op_beq(CPU *cpu, AddrMode mode) {
	cpu_branch(cpu, mode, cpu->z);
}

void cpu_op_bit(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	uint8_t val = cpu_read(cpu, addr);
	cpu->z = (val & cpu->a) == 0;
	cpu->v = (val & 0x40) != 0;
	cpu->n = (val & 0x80) != 0;
}

void cpu_op_bmi(CPU *cpu, AddrMode mode) {
	cpu_branch(cpu, mode, cpu->n);
}

void cpu_op_bne(CPU *cpu, AddrMode mode) {
	cpu_branch(cpu, mode, !cpu->z);
}

void cpu_op_bpl(CPU *cpu, AddrMode mode) {
	cpu_branch(cpu, mode, !cpu->n);
}

void cpu_op_brk(CPU *cpu, AddrMode mode) {
	(void)mode;
	cpu_read(cpu, cpu->pc++);
	cpu_push16(cpu, cpu->pc);
	cpu_push(cpu, cpu->p | 0x30);
	cpu->i = 1;
	cpu->pc = cpu_read16(cpu, 0xfffe);
}

void cpu_op_bvc(CPU *cpu, AddrMode mode) {
	cpu_branch(cpu, mode, !cpu->v);
}

void cpu_op_bvs(CPU *cpu, AddrMode mode) {
	cpu_branch(cpu, mode, cpu->v);
}

void cpu_op_clc(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->c = 0;
}

void cpu_op_cld(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->d = 0;
}

void cpu_op_cli(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->i = 0;
}

void cpu_op_clv(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->v = 0;
}

void cpu_op_cmp(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	uint8_t val = cpu_read(cpu, addr);
	cpu->c = cpu->a >= val;
	val = cpu->a - val;
	cpu_set_zn(cpu, val);
}

void cpu_op_cpx(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	uint8_t val = cpu_read(cpu, addr);
	cpu->c = cpu->x >= val;
	val = cpu->x - val;
	cpu_set_zn(cpu, val);
}

void cpu_op_cpy(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	uint8_t val = cpu_read(cpu, addr);
	cpu->c = cpu->y >= val;
	val = cpu->y - val;
	cpu_set_zn(cpu, val);
}

void cpu_op_dec(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, RMW);
	uint8_t val = cpu_read(cpu, addr);
	cpu_write(cpu, addr, val);
	val--;
	cpu_write(cpu, addr, val);
	cpu_set_zn(cpu, val);
}

void cpu_op_dex(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->x--;
	cpu_set_zn(cpu, cpu->x);
}

void cpu_op_dey(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->y--;
	cpu_set_zn(cpu, cpu->y);
}

void cpu_op_eor(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	cpu->a ^= cpu_read(cpu, addr);
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_inc(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, RMW);
	uint8_t val = cpu_read(cpu, addr);
	cpu_write(cpu, addr, val);
	val++;
	cpu_write(cpu, addr, val);
	cpu_set_zn(cpu, val);
}

void cpu_op_inx(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->x++;
	cpu_set_zn(cpu, cpu->x);
}

void cpu_op_iny(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->y++;
	cpu_set_zn(cpu, cpu->y);
}

void cpu_op_jmp(CPU *cpu, AddrMode mode) {
	cpu->pc = cpu_oper_addr(cpu, mode, READ);
}

void cpu_op_jsr(CPU *cpu, AddrMode mode) {
	cpu_read(cpu, 0x100 + cpu->sp);
	cpu_push16(cpu, cpu->pc + 1);
	cpu->pc = cpu_oper_addr(cpu, mode, READ);
}

void cpu_op_lda(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	cpu->a = cpu_read(cpu, addr);
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_ldx(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	cpu->x = cpu_read(cpu, addr);
	cpu_set_zn(cpu, cpu->x);
}

void cpu_op_ldy(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	cpu->y = cpu_read(cpu, addr);
	cpu_set_zn(cpu, cpu->y);
}

void cpu_op_lsr(CPU *cpu, AddrMode mode) {
	if (mode == ACC) {
		cpu_oper_addr(cpu, mode, READ);
		cpu->c = cpu->a & 1;
		cpu->a >>= 1;
		cpu_set_zn(cpu, cpu->a);
	} else {
		uint16_t addr = cpu_oper_addr(cpu, mode, RMW);
		uint8_t val = cpu_read(cpu, addr);
		cpu_write(cpu, addr, val);
		cpu->c = val & 1;
		val >>= 1;
		cpu_write(cpu, addr, val);
		cpu_set_zn(cpu, val);
	}
}

void cpu_op_nop(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	if (mode != IMP)
		cpu_read(cpu, addr);
}

void cpu_op_ora(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	cpu->a |= cpu_read(cpu, addr);
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_pha(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu_push(cpu, cpu->a);
}

void cpu_op_php(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu_push(cpu, cpu->p | 0x30);
}

void cpu_op_pla(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu_read(cpu, 0x100 | cpu->sp);
	cpu->a = cpu_pull(cpu);
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_plp(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu_read(cpu, 0x100 | cpu->sp);
	cpu->p = (cpu_pull(cpu) & ~0x10) | 0x20;
}

void cpu_op_rol(CPU *cpu, AddrMode mode) {
	if (mode == ACC) {
		cpu_oper_addr(cpu, mode, READ);
		uint8_t oldc = cpu->c;
		cpu->c = (cpu->a & 0x80) != 0;
		cpu->a = (cpu->a << 1) | oldc;
		cpu_set_zn(cpu, cpu->a);
	} else {
		uint16_t addr = cpu_oper_addr(cpu, mode, RMW);
		uint8_t val = cpu_read(cpu, addr);
		cpu_write(cpu, addr, val);
		uint8_t oldc = cpu->c;
		cpu->c = (val & 0x80) != 0;
		val = (val << 1) | oldc;
		cpu_write(cpu, addr, val);
		cpu_set_zn(cpu, val);
	}
}

void cpu_op_ror(CPU *cpu, AddrMode mode) {
	if (mode == ACC) {
		cpu_oper_addr(cpu, mode, READ);
		uint8_t oldc = cpu->c;
		cpu->c = cpu->a & 1;
		cpu->a = (cpu->a >> 1) | (oldc << 7);
		cpu_set_zn(cpu, cpu->a);
	} else {
		uint16_t addr = cpu_oper_addr(cpu, mode, RMW);
		uint8_t val = cpu_read(cpu, addr);
		cpu_write(cpu, addr, val);
		uint8_t oldc = cpu->c;
		cpu->c = val & 1;
		val = (val >> 1) | (oldc << 7);
		cpu_write(cpu, addr, val);
		cpu_set_zn(cpu, val);
	}
}

void cpu_op_rti(CPU *cpu, AddrMode mode) {
	(void)mode;
	cpu_read(cpu, cpu->pc);
	cpu_read(cpu, 0x0100 | cpu->sp);
	cpu->p = (cpu_pull(cpu) & ~0x10) | 0x20;
	cpu->pc = cpu_pull16(cpu);
}

void cpu_op_rts(CPU *cpu, AddrMode mode) {
	(void)mode;
	cpu_read(cpu, cpu->pc);
	cpu_read(cpu, 0x100 + cpu->sp);
	cpu->pc = cpu_pull16(cpu);
	cpu_read(cpu, cpu->pc);
	cpu->pc++;
}

void cpu_op_sbc(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	uint8_t val = cpu_read(cpu, addr);
	val = ~val;
	uint16_t temp = cpu->a + val + cpu->c;
	cpu->v = (~(cpu->a ^ val) & (cpu->a ^ temp) & 0x80) != 0;
	cpu->c = temp > 0xff;
	cpu->a = temp;
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_sec(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->c = 1;
}

void cpu_op_sed(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->d = 1;
}

void cpu_op_sei(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->i = 1;
}

void cpu_op_sta(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, WRITE);
	cpu_write(cpu, addr, cpu->a);
}

void cpu_op_stx(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, WRITE);
	cpu_write(cpu, addr, cpu->x);
}

void cpu_op_sty(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, WRITE);
	cpu_write(cpu, addr, cpu->y);
}

void cpu_op_tax(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->x = cpu->a;
	cpu_set_zn(cpu, cpu->x);
}

void cpu_op_tay(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->y = cpu->a;
	cpu_set_zn(cpu, cpu->y);
}

void cpu_op_tsx(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->x = cpu->sp;
	cpu_set_zn(cpu, cpu->x);
}

void cpu_op_txa(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->a = cpu->x;
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_txs(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->sp = cpu->x;
}

void cpu_op_tya(CPU *cpu, AddrMode mode) {
	cpu_oper_addr(cpu, mode, READ);
	cpu->a = cpu->y;
	cpu_set_zn(cpu, cpu->a);
}

// unofficial opcodes
//
void cpu_op_alr(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	uint8_t val = cpu_read(cpu, addr);
	val &= cpu->a;
	cpu->c = val & 1;
	cpu->a = val >> 1;
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_anc(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	uint8_t val = cpu_read(cpu, addr);
	cpu->a &= val;
	cpu_set_zn(cpu, cpu->a);
	cpu->c = (cpu->a & 0x80) != 0;
}

void cpu_op_ane(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	uint8_t val = cpu_read(cpu, addr);
	cpu->a = cpu->x & cpu->a & val;
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_arr(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	uint8_t val = cpu_read(cpu, addr);
	val &= cpu->a;
	uint8_t result = (val >> 1) | (cpu->c << 7);
	cpu->a = result;
	cpu_set_zn(cpu, cpu->a);
	cpu->c = (cpu->a >> 6) & 1;
	cpu->v = ((cpu->a >> 6) ^ (cpu->a >> 5)) & 1;
}

void cpu_op_dcp(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, RMW);
	uint8_t val = cpu_read(cpu, addr);
	cpu_write(cpu, addr, val);
	val--;
	cpu_write(cpu, addr, val);
	cpu->c = cpu->a >= val;
	uint8_t temp = cpu->a - val;
	cpu_set_zn(cpu, temp);
}

void cpu_op_isc(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, RMW);
	uint8_t val = cpu_read(cpu, addr);
	cpu_write(cpu, addr, val);
	val++;
	cpu_write(cpu, addr, val);
	uint8_t sbc_val = ~val;
	uint16_t temp = cpu->a + sbc_val + cpu->c;
	cpu->v = (~(cpu->a ^ sbc_val) & (cpu->a ^ temp) & 0x80) != 0;
	cpu->c = temp > 0xff;
	cpu->a = temp;
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_las(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	uint8_t val = cpu_read(cpu, addr);
	val &= cpu->sp;
	cpu->a = val;
	cpu->x = val;
	cpu->sp = val;
	cpu_set_zn(cpu, val);
}

void cpu_op_lax(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	uint8_t val = cpu_read(cpu, addr);
	cpu->a = cpu->x = val;
	cpu_set_zn(cpu, val);
}

void cpu_op_lxa(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	uint8_t val = cpu_read(cpu, addr);
	cpu->a = (cpu->a | 0xee) & val;
	cpu->x = cpu->a;
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_rla(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, RMW);
	uint8_t val = cpu_read(cpu, addr);
	cpu_write(cpu, addr, val);
	uint8_t oldc = cpu->c;
	cpu->c = (val & 0x80) != 0;
	val = (val << 1) | oldc;
	cpu_write(cpu, addr, val);
	cpu->a &= val;
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_rra(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, RMW);
	uint8_t val = cpu_read(cpu, addr);
	cpu_write(cpu, addr, val);
	uint8_t oldc = cpu->c;
	cpu->c = val & 1;
	val = (val >> 1) | (oldc << 7);
	cpu_write(cpu, addr, val);
	uint16_t temp = cpu->a + val + cpu->c;
	cpu->v = (~(cpu->a ^ val) & (cpu->a ^ temp) & 0x80) != 0;
	cpu->c = temp > 0xff;
	cpu->a = temp;
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_sax(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, WRITE);
	cpu_write(cpu, addr, cpu->a & cpu->x);
}

void cpu_op_sbx(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, READ);
	uint8_t val = cpu_read(cpu, addr);
	uint8_t and_val = cpu->a & cpu->x;
	uint16_t temp = and_val - val;
	cpu->c = (and_val >= val);
	cpu->x = temp & 0xFF;
	cpu_set_zn(cpu, cpu->x);
}

void cpu_op_sha(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, WRITE);
	uint8_t val = cpu->a & cpu->x & ((addr >> 8) + 1);
	cpu_write(cpu, addr, val);
}

void cpu_op_shx(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, WRITE);
	uint8_t val = cpu->x & ((addr >> 8) + 1);
	cpu_write(cpu, addr, val);
}

void cpu_op_shy(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, WRITE);
	uint8_t val = cpu->y & ((addr >> 8) + 1);
	cpu_write(cpu, addr, val);
}

void cpu_op_slo(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, RMW);
	uint8_t val = cpu_read(cpu, addr);
	cpu_write(cpu, addr, val);
	cpu->c = (val & 0x80) != 0;
	val <<= 1;
	cpu_write(cpu, addr, val);
	cpu->a |= val;
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_sre(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, RMW);
	uint8_t val = cpu_read(cpu, addr);
	cpu_write(cpu, addr, val);
	cpu->c = val & 1;
	val >>= 1;
	cpu_write(cpu, addr, val);
	cpu->a ^= val;
	cpu_set_zn(cpu, cpu->a);
}

void cpu_op_tas(CPU *cpu, AddrMode mode) {
	uint16_t addr = cpu_oper_addr(cpu, mode, WRITE);
	cpu->sp = cpu->a & cpu->x;
	uint8_t val = cpu->sp & ((addr >> 8) + 1);
	cpu_write(cpu, addr, val);
}

void cpu_op_jam(CPU *cpu, AddrMode mode) {
	(void)mode;
	cpu->jammed = 1;
}

void cpu_op_switch(CPU *cpu, uint8_t opcode) {
	switch (opcode) {

	// official opcodes

	// add with carry
	case 0x69: cpu_op_adc(cpu, IMM); break;
	case 0x65: cpu_op_adc(cpu, ZPG); break;
	case 0x75: cpu_op_adc(cpu, ZPX); break;
	case 0x6d: cpu_op_adc(cpu, ABS); break;
	case 0x7d: cpu_op_adc(cpu, ABX); break;
	case 0x79: cpu_op_adc(cpu, ABY); break;
	case 0x61: cpu_op_adc(cpu, INX); break;
	case 0x71: cpu_op_adc(cpu, INY); break;
	// logical and
	case 0x29: cpu_op_and(cpu, IMM); break;
	case 0x25: cpu_op_and(cpu, ZPG); break;
	case 0x35: cpu_op_and(cpu, ZPX); break;
	case 0x2d: cpu_op_and(cpu, ABS); break;
	case 0x3d: cpu_op_and(cpu, ABX); break;
	case 0x39: cpu_op_and(cpu, ABY); break;
	case 0x21: cpu_op_and(cpu, INX); break;
	case 0x31: cpu_op_and(cpu, INY); break;
	// arithmetic shift left
	case 0x0a: cpu_op_asl(cpu, ACC); break;
	case 0x06: cpu_op_asl(cpu, ZPG); break;
	case 0x16: cpu_op_asl(cpu, ZPX); break;
	case 0x0e: cpu_op_asl(cpu, ABS); break;
	case 0x1e: cpu_op_asl(cpu, ABX); break;
	// branches
	case 0x90: cpu_op_bcc(cpu, REL); break;
	case 0xb0: cpu_op_bcs(cpu, REL); break;
	case 0xf0: cpu_op_beq(cpu, REL); break;
	case 0x30: cpu_op_bmi(cpu, REL); break;
	case 0xd0: cpu_op_bne(cpu, REL); break;
	case 0x10: cpu_op_bpl(cpu, REL); break;
	case 0x50: cpu_op_bvc(cpu, REL); break;
	case 0x70: cpu_op_bvs(cpu, REL); break;
	// bit test
	case 0x24: cpu_op_bit(cpu, ZPG); break;
	case 0x2c: cpu_op_bit(cpu, ABS); break;
	// break interrupt
	case 0x00: cpu_op_brk(cpu, IMP); break;
	// clear flags
	case 0x18: cpu_op_clc(cpu, IMP); break;
	case 0xd8: cpu_op_cld(cpu, IMP); break;
	case 0x58: cpu_op_cli(cpu, IMP); break;
	case 0xb8: cpu_op_clv(cpu, IMP); break;
	// compare accumulator
	case 0xc9: cpu_op_cmp(cpu, IMM); break;
	case 0xc5: cpu_op_cmp(cpu, ZPG); break;
	case 0xd5: cpu_op_cmp(cpu, ZPX); break;
	case 0xcd: cpu_op_cmp(cpu, ABS); break;
	case 0xdd: cpu_op_cmp(cpu, ABX); break;
	case 0xd9: cpu_op_cmp(cpu, ABY); break;
	case 0xc1: cpu_op_cmp(cpu, INX); break;
	case 0xd1: cpu_op_cmp(cpu, INY); break;
	// compare x register
	case 0xe0: cpu_op_cpx(cpu, IMM); break;
	case 0xe4: cpu_op_cpx(cpu, ZPG); break;
	case 0xec: cpu_op_cpx(cpu, ABS); break;
	// compare y register
	case 0xc0: cpu_op_cpy(cpu, IMM); break;
	case 0xc4: cpu_op_cpy(cpu, ZPG); break;
	case 0xcc: cpu_op_cpy(cpu, ABS); break;
	// decrement memory
	case 0xc6: cpu_op_dec(cpu, ZPG); break;
	case 0xd6: cpu_op_dec(cpu, ZPX); break;
	case 0xce: cpu_op_dec(cpu, ABS); break;
	case 0xde: cpu_op_dec(cpu, ABX); break;
	// decrement registers
	case 0xca: cpu_op_dex(cpu, IMP); break;
	case 0x88: cpu_op_dey(cpu, IMP); break;
	// exclusive or
	case 0x49: cpu_op_eor(cpu, IMM); break;
	case 0x45: cpu_op_eor(cpu, ZPG); break;
	case 0x55: cpu_op_eor(cpu, ZPX); break;
	case 0x4d: cpu_op_eor(cpu, ABS); break;
	case 0x5d: cpu_op_eor(cpu, ABX); break;
	case 0x59: cpu_op_eor(cpu, ABY); break;
	case 0x41: cpu_op_eor(cpu, INX); break;
	case 0x51: cpu_op_eor(cpu, INY); break;
	// increment memory
	case 0xe6: cpu_op_inc(cpu, ZPG); break;
	case 0xf6: cpu_op_inc(cpu, ZPX); break;
	case 0xee: cpu_op_inc(cpu, ABS); break;
	case 0xfe: cpu_op_inc(cpu, ABX); break;
	// increment registers
	case 0xe8: cpu_op_inx(cpu, IMP); break;
	case 0xc8: cpu_op_iny(cpu, IMP); break;
	// jumps
	case 0x4c: cpu_op_jmp(cpu, ABS); break;
	case 0x6c: cpu_op_jmp(cpu, IND); break;
	case 0x20: cpu_op_jsr(cpu, ABS); break;
	// load accumulator
	case 0xa9: cpu_op_lda(cpu, IMM); break;
	case 0xa5: cpu_op_lda(cpu, ZPG); break;
	case 0xb5: cpu_op_lda(cpu, ZPX); break;
	case 0xad: cpu_op_lda(cpu, ABS); break;
	case 0xbd: cpu_op_lda(cpu, ABX); break;
	case 0xb9: cpu_op_lda(cpu, ABY); break;
	case 0xa1: cpu_op_lda(cpu, INX); break;
	case 0xb1: cpu_op_lda(cpu, INY); break;
	// load x register
	case 0xa2: cpu_op_ldx(cpu, IMM); break;
	case 0xa6: cpu_op_ldx(cpu, ZPG); break;
	case 0xb6: cpu_op_ldx(cpu, ZPY); break;
	case 0xae: cpu_op_ldx(cpu, ABS); break;
	case 0xbe: cpu_op_ldx(cpu, ABY); break;
	// load y register
	case 0xa0: cpu_op_ldy(cpu, IMM); break;
	case 0xa4: cpu_op_ldy(cpu, ZPG); break;
	case 0xb4: cpu_op_ldy(cpu, ZPX); break;
	case 0xac: cpu_op_ldy(cpu, ABS); break;
	case 0xbc: cpu_op_ldy(cpu, ABX); break;
	// logical shift right
	case 0x4a: cpu_op_lsr(cpu, ACC); break;
	case 0x46: cpu_op_lsr(cpu, ZPG); break;
	case 0x56: cpu_op_lsr(cpu, ZPX); break;
	case 0x4e: cpu_op_lsr(cpu, ABS); break;
	case 0x5e: cpu_op_lsr(cpu, ABX); break;
	// no operation
	case 0xea: cpu_op_nop(cpu, IMP); break;
	// logical inclusive or
	case 0x09: cpu_op_ora(cpu, IMM); break;
	case 0x05: cpu_op_ora(cpu, ZPG); break;
	case 0x15: cpu_op_ora(cpu, ZPX); break;
	case 0x0d: cpu_op_ora(cpu, ABS); break;
	case 0x1d: cpu_op_ora(cpu, ABX); break;
	case 0x19: cpu_op_ora(cpu, ABY); break;
	case 0x01: cpu_op_ora(cpu, INX); break;
	case 0x11: cpu_op_ora(cpu, INY); break;
	// stack pushes
	case 0x48: cpu_op_pha(cpu, IMP); break;
	case 0x08: cpu_op_php(cpu, IMP); break;
	// stack pulls
	case 0x68: cpu_op_pla(cpu, IMP); break;
	case 0x28: cpu_op_plp(cpu, IMP); break;
	// rotate left
	case 0x2a: cpu_op_rol(cpu, ACC); break;
	case 0x26: cpu_op_rol(cpu, ZPG); break;
	case 0x36: cpu_op_rol(cpu, ZPX); break;
	case 0x2e: cpu_op_rol(cpu, ABS); break;
	case 0x3e: cpu_op_rol(cpu, ABX); break;
	// rotate right
	case 0x6a: cpu_op_ror(cpu, ACC); break;
	case 0x66: cpu_op_ror(cpu, ZPG); break;
	case 0x76: cpu_op_ror(cpu, ZPX); break;
	case 0x6e: cpu_op_ror(cpu, ABS); break;
	case 0x7e: cpu_op_ror(cpu, ABX); break;
	// returns
	case 0x40: cpu_op_rti(cpu, IMP); break;
	case 0x60: cpu_op_rts(cpu, IMP); break;
	// subtract with carry
	case 0xe9: cpu_op_sbc(cpu, IMM); break;
	case 0xe5: cpu_op_sbc(cpu, ZPG); break;
	case 0xf5: cpu_op_sbc(cpu, ZPX); break;
	case 0xed: cpu_op_sbc(cpu, ABS); break;
	case 0xfd: cpu_op_sbc(cpu, ABX); break;
	case 0xf9: cpu_op_sbc(cpu, ABY); break;
	case 0xe1: cpu_op_sbc(cpu, INX); break;
	case 0xf1: cpu_op_sbc(cpu, INY); break;
	// set flags
	case 0x38: cpu_op_sec(cpu, IMP); break;
	case 0xf8: cpu_op_sed(cpu, IMP); break;
	case 0x78: cpu_op_sei(cpu, IMP); break;
	// store accumulator
	case 0x85: cpu_op_sta(cpu, ZPG); break;
	case 0x95: cpu_op_sta(cpu, ZPX); break;
	case 0x8d: cpu_op_sta(cpu, ABS); break;
	case 0x9d: cpu_op_sta(cpu, ABX); break;
	case 0x99: cpu_op_sta(cpu, ABY); break;
	case 0x81: cpu_op_sta(cpu, INX); break;
	case 0x91: cpu_op_sta(cpu, INY); break;
	// store x register
	case 0x86: cpu_op_stx(cpu, ZPG); break;
	case 0x96: cpu_op_stx(cpu, ZPY); break;
	case 0x8e: cpu_op_stx(cpu, ABS); break;
	// store y register
	case 0x84: cpu_op_sty(cpu, ZPG); break;
	case 0x94: cpu_op_sty(cpu, ZPX); break;
	case 0x8c: cpu_op_sty(cpu, ABS); break;
	// transfer registers
	case 0xaa: cpu_op_tax(cpu, IMP); break;
	case 0xa8: cpu_op_tay(cpu, IMP); break;
	case 0xba: cpu_op_tsx(cpu, IMP); break;
	case 0x8a: cpu_op_txa(cpu, IMP); break;
	case 0x9a: cpu_op_txs(cpu, IMP); break;
	case 0x98: cpu_op_tya(cpu, IMP); break;

	// unofficial opcodes

	// decrement memory and compare
	case 0xc7: cpu_op_dcp(cpu, ZPG); break;
	case 0xd7: cpu_op_dcp(cpu, ZPX); break;
	case 0xcf: cpu_op_dcp(cpu, ABS); break;
	case 0xdf: cpu_op_dcp(cpu, ABX); break;
	case 0xdb: cpu_op_dcp(cpu, ABY); break;
	case 0xc3: cpu_op_dcp(cpu, INX); break;
	case 0xd3: cpu_op_dcp(cpu, INY); break;
	// increment memory and subtract with carry
	case 0xe7: cpu_op_isc(cpu, ZPG); break;
	case 0xf7: cpu_op_isc(cpu, ZPX); break;
	case 0xef: cpu_op_isc(cpu, ABS); break;
	case 0xff: cpu_op_isc(cpu, ABX); break;
	case 0xfb: cpu_op_isc(cpu, ABY); break;
	case 0xe3: cpu_op_isc(cpu, INX); break;
	case 0xf3: cpu_op_isc(cpu, INY); break;
	// jam cpu
	case 0x02: cpu_op_jam(cpu, IMP); break;
	case 0x12: cpu_op_jam(cpu, IMP); break;
	case 0x22: cpu_op_jam(cpu, IMP); break;
	case 0x32: cpu_op_jam(cpu, IMP); break;
	case 0x42: cpu_op_jam(cpu, IMP); break;
	case 0x52: cpu_op_jam(cpu, IMP); break;
	case 0x62: cpu_op_jam(cpu, IMP); break;
	case 0x72: cpu_op_jam(cpu, IMP); break;
	case 0x92: cpu_op_jam(cpu, IMP); break;
	case 0xb2: cpu_op_jam(cpu, IMP); break;
	case 0xd2: cpu_op_jam(cpu, IMP); break;
	case 0xf2: cpu_op_jam(cpu, IMP); break;
	// load accumulator and x register
	case 0xa7: cpu_op_lax(cpu, ZPG); break;
	case 0xb7: cpu_op_lax(cpu, ZPY); break;
	case 0xaf: cpu_op_lax(cpu, ABS); break;
	case 0xbf: cpu_op_lax(cpu, ABY); break;
	case 0xa3: cpu_op_lax(cpu, INX); break;
	case 0xb3: cpu_op_lax(cpu, INY); break;
	// no operation
	case 0x1a: cpu_op_nop(cpu, IMP); break;
	case 0x3a: cpu_op_nop(cpu, IMP); break;
	case 0x5a: cpu_op_nop(cpu, IMP); break;
	case 0x7a: cpu_op_nop(cpu, IMP); break;
	case 0xda: cpu_op_nop(cpu, IMP); break;
	case 0xfa: cpu_op_nop(cpu, IMP); break;
	case 0x80: cpu_op_nop(cpu, IMM); break;
	case 0x82: cpu_op_nop(cpu, IMM); break;
	case 0x89: cpu_op_nop(cpu, IMM); break;
	case 0xc2: cpu_op_nop(cpu, IMM); break;
	case 0xe2: cpu_op_nop(cpu, IMM); break;
	case 0x04: cpu_op_nop(cpu, ZPG); break;
	case 0x44: cpu_op_nop(cpu, ZPG); break;
	case 0x64: cpu_op_nop(cpu, ZPG); break;
	case 0x14: cpu_op_nop(cpu, ZPX); break;
	case 0x34: cpu_op_nop(cpu, ZPX); break;
	case 0x54: cpu_op_nop(cpu, ZPX); break;
	case 0x74: cpu_op_nop(cpu, ZPX); break;
	case 0xd4: cpu_op_nop(cpu, ZPX); break;
	case 0xf4: cpu_op_nop(cpu, ZPX); break;
	case 0x0c: cpu_op_nop(cpu, ABS); break;
	case 0x1c: cpu_op_nop(cpu, ABX); break;
	case 0x3c: cpu_op_nop(cpu, ABX); break;
	case 0x5c: cpu_op_nop(cpu, ABX); break;
	case 0x7c: cpu_op_nop(cpu, ABX); break;
	case 0xdc: cpu_op_nop(cpu, ABX); break;
	case 0xfc: cpu_op_nop(cpu, ABX); break;
	// rotate left and and
	case 0x27: cpu_op_rla(cpu, ZPG); break;
	case 0x37: cpu_op_rla(cpu, ZPX); break;
	case 0x2f: cpu_op_rla(cpu, ABS); break;
	case 0x3f: cpu_op_rla(cpu, ABX); break;
	case 0x3b: cpu_op_rla(cpu, ABY); break;
	case 0x23: cpu_op_rla(cpu, INX); break;
	case 0x33: cpu_op_rla(cpu, INY); break;
	// rotate right and add with carry
	case 0x67: cpu_op_rra(cpu, ZPG); break;
	case 0x77: cpu_op_rra(cpu, ZPX); break;
	case 0x6f: cpu_op_rra(cpu, ABS); break;
	case 0x7f: cpu_op_rra(cpu, ABX); break;
	case 0x7b: cpu_op_rra(cpu, ABY); break;
	case 0x63: cpu_op_rra(cpu, INX); break;
	case 0x73: cpu_op_rra(cpu, INY); break;
	// store accumulator & x register
	case 0x87: cpu_op_sax(cpu, ZPG); break;
	case 0x97: cpu_op_sax(cpu, ZPY); break;
	case 0x8f: cpu_op_sax(cpu, ABS); break;
	case 0x83: cpu_op_sax(cpu, INX); break;
	// shift left and or
	case 0x07: cpu_op_slo(cpu, ZPG); break;
	case 0x17: cpu_op_slo(cpu, ZPX); break;
	case 0x0f: cpu_op_slo(cpu, ABS); break;
	case 0x1f: cpu_op_slo(cpu, ABX); break;
	case 0x1b: cpu_op_slo(cpu, ABY); break;
	case 0x03: cpu_op_slo(cpu, INX); break;
	case 0x13: cpu_op_slo(cpu, INY); break;
	// shift right and exclusive or
	case 0x47: cpu_op_sre(cpu, ZPG); break;
	case 0x57: cpu_op_sre(cpu, ZPX); break;
	case 0x4f: cpu_op_sre(cpu, ABS); break;
	case 0x5f: cpu_op_sre(cpu, ABX); break;
	case 0x5b: cpu_op_sre(cpu, ABY); break;
	case 0x43: cpu_op_sre(cpu, INX); break;
	case 0x53: cpu_op_sre(cpu, INY); break;
	// single case unofficial
	case 0x4b: cpu_op_alr(cpu, IMM); break;
	case 0x0b: cpu_op_anc(cpu, IMM); break;
	case 0x2b: cpu_op_anc(cpu, IMM); break;
	case 0x8b: cpu_op_ane(cpu, IMM); break;
	case 0x6b: cpu_op_arr(cpu, IMM); break;
	case 0xbb: cpu_op_las(cpu, ABY); break;
	case 0xab: cpu_op_lxa(cpu, IMM); break;
	case 0xeb: cpu_op_sbc(cpu, IMM); break;
	case 0xcb: cpu_op_sbx(cpu, IMM); break;
	case 0x9f: cpu_op_sha(cpu, ABY); break;
	case 0x93: cpu_op_sha(cpu, INY); break;
	case 0x9e: cpu_op_shx(cpu, ABY); break;
	case 0x9c: cpu_op_shy(cpu, ABX); break;
	case 0x9b: cpu_op_tas(cpu, ABY); break;

	default:
		lerror("cpu_op_switch: unimplemented opcode: 0x%02x\n", opcode);
		host_exit(1);
	}
}


void cpu_poweron(CPU *cpu) {
	// accurate poweron vs reset later
	cpu_reset(cpu);
}

void cpu_reset(CPU *cpu) {
	linfo("reset cpu\n");
	cpu->a = 0;
	cpu->x = 0;
	cpu->y = 0;
	cpu->sp = 0xfd;
	cpu->p = 0x24;
	cpu->pc = cpu_read16(cpu, 0xFFFC);
	cpu->jammed = 0;
	cpu->cycles = 7;
}

void cpu_step(CPU *cpu) {

	if (cpu->jammed) {
		return;
	}

	uint8_t opcode = cpu_read(cpu, cpu->pc++);

	/* // logging for nestest
	char binp[9] = {0};
	for (int i = 0; i < 8; i++){
		binp[7 - i] = (cpu->p >> i) & 1 ? '1' : '0';
	}

	if (!(cpu->cycles % 1789773)) // one clock second
	printf(
		"cpu: %04x %02x a:%02x x:%02x y:%02x p:%s sp:%02x cyc:%ld\n",
		(uint16_t)(cpu->pc-1),
		opcode,
		cpu->a,
		cpu->x,
		cpu->y,
		binp,
		cpu->sp,
		cpu->cycles - 1
	);
	fflush(stdout);
	*/
	cpu_op_switch(cpu, opcode);
}

void cpu_run(CPU *cpu) {
	// runs until vblank
	while (true) {
		uint64_t cycles_before = cpu->cycles;
		cpu_step(cpu);
		if (bus_clock(cpu->bus, cpu->cycles - cycles_before))
			return; // allow main to handle calls
	}
}
