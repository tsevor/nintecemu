#include <stdio.h>

#include "bus.h"
#include "cart.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "host.h"

int loglevel = 3;

int main(int argc, char **argv) {

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <rom.nes>\n", argv[0]);
		return 1;
	}

	Cart cart;

	if (cart_load(&cart, argv[1])) {
		fprintf(stderr, "Failed to load cart: %s\n", argv[1]);
		return 1;
	}

	int hecode = host_init();

	if (hecode) {
		fprintf(stderr, "Failed to init host interface: %d\n", hecode);
		return 1;
	}

	// declare components
	Bus bus;
	CPU cpu;
	PPU ppu;
	APU apu;

	// connect components
	cpu.bus = &bus;

	bus.cart = &cart;
	bus.ppu = &ppu;
	bus.apu = &apu;

	ppu.cart = &cart;

	// init components
	bus_init(&bus);
	ppu_init(&ppu);
	apu_init(&apu);
	cpu_poweron(&cpu);

	while (1) {
		cpu_run(&cpu);
		// vblank
		host_frame();
	}
}
