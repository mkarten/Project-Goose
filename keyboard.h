#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "stdint.h"
#include "stdbool.h"
#include "memory.h"
#include "io.h"

// Key states
#define KEY_RELEASED 0
#define KEY_JUST_PRESSED 1
#define KEY_HELD 2
#define KEY_JUST_RELEASED 3

// Special keys
#define KEY_ESCAPE 0x01
#define KEY_ENTER 0x1C
#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36
#define KEY_LCTRL 0x1D
#define KEY_CAPS_LOCK 0x3A
#define KEY_ALT_GR 0x38  // Right Alt key for French layout

// Layout identifiers
#define LAYOUT_US 0
#define LAYOUT_FR 1

// Keyboard layout structure
typedef struct {
    char* name;
    unsigned char* scancode_to_ascii;
    unsigned char* scancode_to_ascii_shift;
    unsigned char* scancode_to_ascii_altgr;  // For French special characters
} KeyboardLayout;

// Keyboard state structure
typedef struct {
    uint8_t* key_states;           // Current state of each key
    uint8_t* previous_key_states;  // Previous state of each key
    bool shift_pressed;
    bool ctrl_pressed;
    bool caps_lock;
    bool altgr_pressed;           // For French layout
    KeyboardLayout* current_layout;
    KeyboardLayout** available_layouts;
    uint32_t layout_count;
} KeyboardState;

extern KeyboardState* kb_state;
extern uint8_t last_scancode;

// Initialize keyboard system
void keyboard_init();

// Keyboard interrupt handler
void keyboard_handler();

// Get current key state
uint8_t keyboard_get_key_state(uint8_t scancode);

// Check if key was just pressed this frame
bool keyboard_is_key_pressed(uint8_t scancode);

// Check if key is being held
bool keyboard_is_key_held(uint8_t scancode);

// Check if key was just released this frame
bool keyboard_is_key_released(uint8_t scancode);

// Get ASCII character for current scancode
char keyboard_get_char(uint8_t scancode);

// Get the last pressed character
char keyboard_get_last_char();

// Switch to next keyboard layout
void keyboard_next_layout();

// Get current layout name
const char* keyboard_get_current_layout_name();

#endif // KEYBOARD_H