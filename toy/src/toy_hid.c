#include "include/toy_hid.h"
#include "toy_assert.h"

#include <string.h>


void toy_init_mouse (toy_mouse_t* mouse) {
	memset(mouse, 0, sizeof(*mouse));
}

void toy_set_mouse_key (toy_mouse_t* mouse, enum toy_mouse_key_t key, int value) {
	TOY_ASSERT(0 <= key && key < TOY_MOUSE_KEY_MAX);
	mouse->state.key_state[key] = value;
}


int toy_get_mouse_key (toy_mouse_t* mouse, enum toy_mouse_key_t key) {
	TOY_ASSERT(0 <= key && key < TOY_MOUSE_KEY_MAX);
	return mouse->state.key_state[key];
}


bool toy_is_mouse_key_pressed (toy_mouse_t* mouse, enum toy_mouse_key_t key) {
	TOY_ASSERT(0 <= key && key < TOY_MOUSE_KEY_MAX);
	return (TOY_KEY_STATE_DOWN == mouse->state.key_state[key]) &&
		(TOY_KEY_STATE_UP == mouse->old_state.key_state[key]);
}


bool toy_is_mouse_key_released (toy_mouse_t* mouse, enum toy_mouse_key_t key) {
	TOY_ASSERT(0 <= key && key < TOY_MOUSE_KEY_MAX);
	return (TOY_KEY_STATE_UP == mouse->state.key_state[key]) &&
		(TOY_KEY_STATE_DOWN == mouse->old_state.key_state[key]);
}


void toy_swap_mouse_state (toy_mouse_t* mouse) {
	mouse->old_state = mouse->state;
}


void toy_init_keyboard (toy_keyboard_t* keyboard) {
	memset(keyboard, 0, sizeof(*keyboard));
}

void toy_swap_keyboard_state (toy_keyboard_t* keyboard) {
	keyboard->old_state = keyboard->state;
}
