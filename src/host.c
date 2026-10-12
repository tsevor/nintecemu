// backend to blind interface
// must be entirely self-contained

#include <stdint.h>
#include <stdlib.h>

#include "host.h"
#include "log.h"

#define WIN_WIDTH 256
#define WIN_HEIGHT 240

#define PIX_SCALE 2

#define WIN_TITLE "nintecemu"

typedef struct Host {
} Host;

// global in window file so other files don't have to pass
// static Host host;

int host_init() {
	return 0;
}

void host_frame() {
}

void host_exit(int code) {
	exit(code);
}
