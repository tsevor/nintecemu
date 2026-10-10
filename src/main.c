#include <stdio.h>

#include "bus.h"
#include "cpu.h"
#include "cart.h"

int loglevel = 3;

int main(int argc, char **argv) {

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <rom.nes>\n", argv[0]);
		return 1;
	}

	Cart *cart = malloc(sizeof(Cart));

	if (cart_load(cart, argv[1])) {
		fprintf(stderr, "Failed to load cart: %s\n", argv[1]);
		return 1;
	}

	Bus bus;
	CPU cpu;

	bus_init(&bus);

	cpu.bus = &bus;
	bus.cart = cart;

	cpu_run(&cpu);


	free(cart);
}
