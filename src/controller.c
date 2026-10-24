#include "controller.h"

uint8_t controller_read(Controller *ctrl) {
	uint8_t ret = 0;
	if (ctrl->read_idx < 8) {
		ret = (ctrl->state.raw >> ctrl->read_idx) & 1;
	} else {
		ret = 1; // Open bus behavior
	}
	if (!ctrl->strobe && ctrl->read_idx < 8) {
		ctrl->read_idx++;
	}
	return ret;
}

void controller_write(Controller *ctrl, uint8_t val) {
	ctrl->strobe = val & 1;
	if (ctrl->strobe) {
		ctrl->read_idx = 0;
	}
}
