#include "bus.h"
#include "log.h"

void bus_init(Bus *bus) {
	for (int i = 0; i < 0x800; i++)
		bus->memory[i] = 0;
}

bool bus_clock(Bus *bus, int cycles) {
	// cpu is master clock / 12
	// ppu is master clock / 4
	// apu is master clock / 12
	// cpu sends in its clock
	apu_clock(bus->apu, cycles);
	return ppu_clock(bus->ppu, cycles);
}

uint8_t bus_read(Bus *bus, uint16_t addr) {
	if (addr < 0x2000) {
		bus->open_bus = bus->memory[addr & 0x7ff];
	} else if (addr >= 0x8000) {
		bus->open_bus = cart_read_prg(bus->cart, addr);
	}
	ldeb2("bus_read: addr=0x%04x val=0x%02x\n", addr, bus->open_bus);
	return bus->open_bus;
}

void bus_write(Bus *bus, uint16_t addr, uint8_t val) {
	ldeb2("bus_write: addr=0x%04x val=0x%02x\n", addr, val);
	if (addr < 0x2000) {
		bus->memory[addr & 0x7ff] = val;
	} else if (addr >= 0x8000) {
		cart_write_prg(bus->cart, addr, val);
	}
}
