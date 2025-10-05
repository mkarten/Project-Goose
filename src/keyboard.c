#include "../includes/keyboard.h"
#include "../includes/kernelStruct.h"
#include "../includes/memory.h"

KeyboardState* kb_state;

// US QWERTY layout
static unsigned char layout_us[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

static unsigned char layout_us_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

// French AZERTY layout
static unsigned char layout_fr[] = {
    0, 0, '&', 'e', '"', '\'', '(', '-', 'e', '_', 'c', 'a', ')', '=', '\b',
    '\t', 'a', 'z', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '^', '$', '\n',
    0, 'q', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 'u', '`',
    0, '*', 'w', 'x', 'c', 'v', 'b', 'n', ',', ';', ':', '!', 0, '*', 0, ' '
};

static unsigned char layout_fr_shift[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'o', '+', '\b',
    '\t', 'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '"', ' ', '\n',
    0, 'Q', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', '%', '~',
    0, ' ', 'W', 'X', 'C', 'V', 'B', 'N', '?', '.', '/', ' ', 0, '*', 0, ' '
};

static unsigned char layout_fr_altgr[] = {
    0, 0, 0, '~', '#', '{', '[', '|', '`', '\\', '^', '@', ']', '}', '\b',
    '\t', 0, 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, ' ', '\n',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '*', 0, ' '
};

uint8_t last_scancode = 0;
static char last_char = 0;  // Add this variable to store the last character

void keyboard_init() {
    kb_state = kmalloc(sizeof(KeyboardState));
    kb_state->key_states = kmalloc(128);
    kb_state->previous_key_states = kmalloc(128);
    
    // Clear states
    for (int i = 0; i < 128; i++) {
        kb_state->key_states[i] = KEY_RELEASED;
        kb_state->previous_key_states[i] = KEY_RELEASED;
    }
    
    // Initialize flags
    kb_state->shift_pressed = false;
    kb_state->ctrl_pressed = false;
    kb_state->caps_lock = false;
    kb_state->altgr_pressed = false;
    
    // Initialize layouts
    kb_state->layout_count = 2;  // US and French layouts
    kb_state->available_layouts = kmalloc(sizeof(KeyboardLayout*) * kb_state->layout_count);
    
    // Create US layout
    KeyboardLayout* us_layout = kmalloc(sizeof(KeyboardLayout));
    us_layout->name = "US-QWERTY";
    us_layout->scancode_to_ascii = layout_us;
    us_layout->scancode_to_ascii_shift = layout_us_shift;
    us_layout->scancode_to_ascii_altgr = 0;  // US layout doesn't use AltGr
    
    // Create French layout
    KeyboardLayout* fr_layout = kmalloc(sizeof(KeyboardLayout));
    fr_layout->name = "FR-AZERTY";
    fr_layout->scancode_to_ascii = layout_fr;
    fr_layout->scancode_to_ascii_shift = layout_fr_shift;
    fr_layout->scancode_to_ascii_altgr = layout_fr_altgr;
    
    kb_state->available_layouts[0] = us_layout;
    kb_state->available_layouts[1] = fr_layout;
    kb_state->current_layout = us_layout;
}

void keyboard_handler() {

    unsigned char scancode = inportb(0x60);
    last_scancode = scancode;
    
    // Store previous state
    kb_state->previous_key_states[scancode & 0x7F] = kb_state->key_states[scancode & 0x7F];
    
    // If this is a key press (not a release), store the character
    if (!(scancode & 0x80)) {
        last_char = keyboard_get_char(scancode);
    }

    // Handle special keys
    if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
        kb_state->shift_pressed = true;
    } else if (scancode == (KEY_LSHIFT | 0x80) || scancode == (KEY_RSHIFT | 0x80)) {
        kb_state->shift_pressed = false;
    } else if (scancode == KEY_LCTRL) {
        kb_state->ctrl_pressed = true;
    } else if (scancode == (KEY_LCTRL | 0x80)) {
        kb_state->ctrl_pressed = false;
    } else if (scancode == KEY_ALT_GR) {
        kb_state->altgr_pressed = true;
    } else if (scancode == (KEY_ALT_GR | 0x80)) {
        kb_state->altgr_pressed = false;
    } else if (scancode == KEY_CAPS_LOCK) {
        kb_state->caps_lock = !kb_state->caps_lock;
    }
    
    // Update key state
    if (scancode & 0x80) {  // Key released
        scancode &= 0x7F;   // Clear release bit
        if (kb_state->key_states[scancode] == KEY_JUST_PRESSED || 
            kb_state->key_states[scancode] == KEY_HELD) {
            kb_state->key_states[scancode] = KEY_JUST_RELEASED;
        }
    } else {  // Key pressed
        if (kb_state->key_states[scancode] == KEY_RELEASED || 
            kb_state->key_states[scancode] == KEY_JUST_RELEASED) {
            kb_state->key_states[scancode] = KEY_JUST_PRESSED;
        } else if (kb_state->key_states[scancode] == KEY_JUST_PRESSED) {
            kb_state->key_states[scancode] = KEY_HELD;
        }
    }
}

uint8_t keyboard_get_key_state(uint8_t scancode) {
    return kb_state->key_states[scancode];
}

bool keyboard_is_key_pressed(uint8_t scancode) {
    return kb_state->key_states[scancode] == KEY_JUST_PRESSED;
}

bool keyboard_is_key_held(uint8_t scancode) {
    return kb_state->key_states[scancode] == KEY_HELD;
}

bool keyboard_is_key_released(uint8_t scancode) {
    return kb_state->key_states[scancode] == KEY_JUST_RELEASED;
}

char keyboard_get_char(uint8_t scancode) {
    if (scancode >= 128) return 0;
    
    // Check for AltGr first (French layout special characters)
    if (kb_state->altgr_pressed && kb_state->current_layout->scancode_to_ascii_altgr) {
        char c = kb_state->current_layout->scancode_to_ascii_altgr[scancode];
        if (c) return c;
    }
    
    bool shift = kb_state->shift_pressed;
    if (kb_state->caps_lock) {
        // Caps lock inverts shift for letters only
        if ((scancode >= 0x10 && scancode <= 0x19) ||  // Q to P
            (scancode >= 0x1E && scancode <= 0x26) ||  // A to L
            (scancode >= 0x2C && scancode <= 0x32)) {  // Z to M
            shift = !shift;
        }
    }
    
    if (shift) {
        return kb_state->current_layout->scancode_to_ascii_shift[scancode];
    } else {
        return kb_state->current_layout->scancode_to_ascii[scancode];
    }
}

char keyboard_get_last_char() {
    return last_char;
}

void keyboard_next_layout() {
    static uint32_t current_layout_index = 0;
    current_layout_index = (current_layout_index + 1) % kb_state->layout_count;
    kb_state->current_layout = kb_state->available_layouts[current_layout_index];
}

const char* keyboard_get_current_layout_name() {
    return kb_state->current_layout->name;
}