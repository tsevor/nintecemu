#include "bus.h"
#include "log.h"

void bus_init(Bus *bus) {
	for (int i = 0; i < 0x800; i++)
		bus->memory[i] = 0;
}

bool bus_clock(Bus *bus, int cycles) {
	apu_clock(bus->apu, cycles);
	return ppu_clock(bus->ppu, cycles * 3);
}

uint8_t bus_read(Bus *bus, uint16_t addr) {
	switch (addr) {
	case 0x0000 ... 0x1fff:
		bus->open_bus = bus->memory[addr & 0x7ff];
		break;
	case 0x2000:
	case 0x2001:
	case 0x2003:
	case 0x2005:
	case 0x2006:
		// write-only ppu register
		bus->open_bus = bus->ppu_open_bus;
		break;
	case 0x2002:
		bus->open_bus = bus->ppu_open_bus =
			(ppu_read_stat(bus->ppu) & 0xe0) | (bus->ppu_open_bus & 0x1f);
			// the bit magic is because the lower 5 bits of ppustat are unused
		break;
	case 0x2004:
		bus->open_bus = bus->ppu_open_bus =
			ppu_read_oamdata(bus->ppu);
		break;
	case 0x2007:
		bus->open_bus = bus->ppu_open_bus =
			ppu_read(bus->ppu);
		break;
	case 0x2008 ... 0x3FFF: // ppu registers mirror
		bus_read(bus, 0x2000 | (addr & 0x7));
		break;
	case 0x4016:
		if (bus->controller1)
			bus->open_bus = (controller_read(bus->controller1) & 0x1f) | (bus->open_bus & 0xe0);
		break;
	case 0x4017:
		if (bus->controller2)
			bus->open_bus = (controller_read(bus->controller2) & 0x1f) | (bus->open_bus & 0xe0);
		break;
	case 0x8000 ... 0xffff:
		bus->open_bus = cart_read_prg(bus->cart, addr);
		break;
	default:
		ldeb2("bus_read: openbus/unmapped: 0x%04x\n", addr);
		break;
	}
	return bus->open_bus;
}

void bus_write(Bus *bus, uint16_t addr, uint8_t val) {
	bus->open_bus = val;
	if (addr >= 0x2000 && addr <= 0x3fff)
		bus->ppu_open_bus = val;

	switch (addr) {
	case 0x0000 ... 0x1fff:
		bus->memory[addr & 0x7ff] = val;
		break;
	case 0x2000:
		ppu_write_ctrl(bus->ppu, val);
		break;
	case 0x2001:
		ppu_write_mask(bus->ppu, val);
		break;
	case 0x2003:
		ppu_write_oamaddr(bus->ppu, val);
		break;
	case 0x2004:
		ppu_write_oamdata(bus->ppu, val);
		break;
	case 0x2005:
		ppu_write_scrl(bus->ppu, val);
		break;
	case 0x2006:
		ppu_write_addr(bus->ppu, val);
		break;
	case 0x2007:
		ppu_write(bus->ppu, val);
		break;
	case 0x2008 ... 0x3FFF:
		bus_write(bus, 0x2000 | (addr & 0x7), val);
		break;
	case 0x4014: // oam dma
		{
			uint8_t data[256];
			uint16_t start_addr = val << 8;
			for (int i = 0; i < 256; i++) {
				data[i] = bus_read(bus, start_addr | (i & 0xff));
			}
			ppu_oam_dma(bus->ppu, data);
			bus->cpu->stall += 513 + (bus->cpu->cycles % 2);
			break;
		}
	case 0x4015:
		// apu
		break;
	case 0x4016:
		if (bus->controller1) controller_write(bus->controller1, val);
		if (bus->controller2) controller_write(bus->controller2, val);
		break;
	case 0x8000 ... 0xffff:
		cart_write_prg(bus->cart, addr, val);
		break;
	default:
		ldeb2("bus_write: openbus/unmapped: 0x%04x\n", addr);
		break;
	}
}
