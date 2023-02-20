#pragma once

#include "toy_platform.h"
#include <stdint.h>
#include <stdbool.h>


/* Human Interface Device */

#define TOY_KEY_STATE_UP 0
#define TOY_KEY_STATE_DOWN 1

typedef struct toy_keyboard_t {
	struct {
		char key_state[UINT8_MAX + 1];
	} state, old_state;
}toy_keyboard_t;


enum toy_mouse_key_t {
	TOY_MOUSE_KEY_LEFT,
	TOY_MOUSE_KEY_RIGHT,
	TOY_MOUSE_KEY_MIDDLE,

	TOY_MOUSE_KEY_MAX
};

typedef struct toy_mouse_t {
	struct {
		int pos_x, pos_y;
		int roll_steps;
		char key_state[TOY_MOUSE_KEY_MAX];
	} state, old_state;
}toy_mouse_t;


typedef struct toy_hid_t {
	toy_mouse_t mouse;
	toy_keyboard_t keyboard;
} toy_hid_t;


TOY_EXTERN_C_START

void toy_init_mouse (toy_mouse_t* mouse);

toy_inline void toy_set_mouse_position (toy_mouse_t* mouse, int x, int y) {
	mouse->state.pos_x = x;
	mouse->state.pos_y = y;
}

toy_inline void toy_get_mouse_position (toy_mouse_t* mouse, int* x, int* y) {
	*x = mouse->state.pos_x;
	*y = mouse->state.pos_y;
}

toy_inline void toy_set_mouse_roll_steps (toy_mouse_t* mouse, int steps) {
	mouse->state.roll_steps = steps;
}

toy_inline int toy_get_mouse_roll_steps (toy_mouse_t* mouse) {
	return mouse->state.roll_steps;
}

void toy_set_mouse_key (toy_mouse_t* mouse, enum toy_mouse_key_t key, int value);

int toy_get_mouse_key (toy_mouse_t* mouse, enum toy_mouse_key_t key);

bool toy_is_mouse_key_pressed (toy_mouse_t* mouse, enum toy_mouse_key_t key);

bool toy_is_mouse_key_released (toy_mouse_t* mouse, enum toy_mouse_key_t key);

toy_inline void toy_roll_mouse (toy_mouse_t* mouse, int steps) {
	mouse->state.roll_steps = steps;
}

void toy_swap_mouse_state (toy_mouse_t* mouse);

void toy_init_keyboard (toy_keyboard_t* keyboard);

toy_inline void toy_set_keyboard_key (toy_keyboard_t* keyboard, uint8_t key, char value) {
	keyboard->state.key_state[key] = value;
}

/*
 * For Windows:
 * https://docs.microsoft.com/zh-cn/windows/win32/inputdev/virtual-key-codes
 * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
 * 0x3A - 0x40 : unassigned
 * VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
 */

toy_inline char toy_get_keyboard_key (toy_keyboard_t* keyboard, uint8_t key) {
	return keyboard->state.key_state[key];
}

toy_inline bool toy_is_keyboard_key_pressed (toy_keyboard_t* keyboard, uint8_t key) {
	return (TOY_KEY_STATE_DOWN == keyboard->state.key_state[key]) &&
		(TOY_KEY_STATE_UP == keyboard->old_state.key_state[key]);
}

toy_inline bool toy_is_keyboard_key_released (toy_keyboard_t* keyboard, uint8_t key) {
	return (TOY_KEY_STATE_UP == keyboard->state.key_state[key]) &&
		(TOY_KEY_STATE_DOWN == keyboard->old_state.key_state[key]);
}

void toy_swap_keyboard_state (toy_keyboard_t* keyboard);


toy_inline void toy_init_hid (toy_hid_t* hid) {
	toy_init_mouse(&hid->mouse);
	toy_init_keyboard(&hid->keyboard);
}


toy_inline void toy_synchronize_hid (toy_hid_t* hid) {
	toy_swap_mouse_state(&hid->mouse);
	toy_swap_keyboard_state(&hid->keyboard);
}

TOY_EXTERN_C_END
