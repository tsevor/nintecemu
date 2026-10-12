#pragma once

#include <stdint.h>

// everything in here should be a blind interface

// internal state struct
typedef struct Host Host;

typedef enum ControllerButton {
	BUTTON_A,
	BUTTON_B,
	BUTTON_SELECT,
	BUTTON_START,
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_LEFT,
	BUTTON_RIGHT
} ControllerButton;

typedef enum EmuCtlEvent {
	E_NONE,
	E_PAUSE,
	E_RESUME,
} EmuCtlEvent;

typedef enum InfoType {
	I_NONE,
	I_PAUSE,
	I_RESUME,

	I_CART_FAIL,
	I_CART_SUCCESS,

	I_CPU_JAM,

	I_MISC

} InfoType;

// called on emulator start
int host_init();

// called on vblank for host event processing
void host_frame();

// called on emulator exit
void host_exit(int code);

// sets pixel color for emulator output
void host_set_pixel();

// sends info for host to display
void host_send_info(InfoType type, char* str);

// gets emulator event, returns true if there are more
bool host_get_event(EmuCtlEvent *event);

// returns controller state
uint8_t host_get_controller();
