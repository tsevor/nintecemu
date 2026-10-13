#pragma once

#include <stdint.h>
#include <stdbool.h>

// everything in here should be a blind interface

// internal state struct
typedef struct Host Host;

typedef enum ControllerButton {
	BUTTON_A      = 0x80,
	BUTTON_B      = 0x40,
	BUTTON_SELECT = 0x20,
	BUTTON_START  = 0x10,
	BUTTON_UP     = 0x08,
	BUTTON_DOWN   = 0x04,
	BUTTON_LEFT   = 0x02,
	BUTTON_RIGHT  = 0x01
} ControllerButton;

typedef enum EmuCtlEventType {
	E_NONE,
	E_LOADROM,
	E_RESET
} EmuCtlEventType;

typedef struct EmuCtlEvent {
	EmuCtlEventType type;
	void* arg;
} EmuCtlEvent;

typedef enum InfoType {
	I_NONE,

	I_CART_FAIL,
	I_CART_SUCCESS,

	I_CPU_RESET,
	I_CPU_JAM,

	I_MISC
} InfoType;

// called on emulator start
int host_init();

// called on vblank for host event processing
void host_frame();

// called on emulator exit
void host_exit(int code);

// sets display for emulator output
void host_set_pixel(int x, int y, uint32_t color);
void host_set_row(int y, uint32_t *buffer);
void host_trade_buffer(uint32_t **buffer);

// sends info for host to display
void host_send_info(InfoType type, char *str);

// gets emulator event, returns true if there is one
bool host_get_event(EmuCtlEvent *event);

// returns controller state
uint8_t host_get_controller();
