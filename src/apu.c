#include "apu.h"
#include "host.h"

void apu_init(APU *apu) {
	apu->cps_counter = 0;
}



void apu_clock(APU *apu, int cycles) {
	for (int i = 0; i < cycles; i++) {

		// channel clocking logic

		apu->cps_counter++;
		// 44.1 khz
		if (apu->cps_counter > (29830/735)) {

			// channel mixing logic

			int16_t sample = 0;
			
			apu->cps_counter = 0;
			host_queue_audio_buffer(&sample, 1);
		}
	}
}

void apu_reset(APU *apu) {
	(void)apu;
}

bool apu_irq_active(APU *apu) {
	(void)apu;
	return false;
}
