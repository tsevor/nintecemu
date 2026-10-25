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

	Cart cart = {0};

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
	Controller controller = {0};
	PPU ppu = { .cart = &cart };
	APU apu;
	Bus bus = {
		.cart = &cart,
		.ppu = &ppu,
		.apu = &apu,
		.controller1 = &controller,
	};
	CPU cpu = { .bus = &bus };

	bus.cpu = &cpu;

	// init components
	bus_init(&bus);
	ppu_init(&ppu);
	apu_init(&apu);
	cpu_poweron(&cpu);


	while (1) {
		cpu_run(&cpu);
		// vblank
		host_frame();
		controller.state.raw = host_get_controller();
		EmuCtlEvent e;
		while (host_get_event(&e)) {
			switch (e.type) {
			case E_RESET:
				bus_reset(&bus);
				host_send_info(I_INFO, "reset system");
				break;
			default:
				break;
			}
		}
	}
}
