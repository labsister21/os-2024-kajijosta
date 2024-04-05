#include "header/cpu/keyboard.h"
#include "header/text/framebuffer.h"
#include "header/cpu/portio.h"
#include "header/stdlib/string.h"

static struct KeyboardDriverState keyboard_state = {
    .keyboard_input_on = false,
    .keyboard_buffer = 0,
    .keyboard_row = 0,
    .keyboard_col = 0,
    .print_mode = false,
};

uint8_t line_lengths[MAX_ROWS] = {0};

const char keyboard_scancode_1_to_ascii_map[256] = {
    0,
    0x1B,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    '\b',
    '\t',
    'q',
    'w',
    'e',
    'r',
    't',
    'y',
    'u',
    'i',
    'o',
    'p',
    '[',
    ']',
    '\n',
    0,
    'a',
    's',
    'd',
    'f',
    'g',
    'h',
    'j',
    'k',
    'l',
    ';',
    '\'',
    '`',
    0,
    '\\',
    'z',
    'x',
    'c',
    'v',
    'b',
    'n',
    'm',
    ',',
    '.',
    '/',
    0,
    '*',
    0,
    ' ',
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    '-',
    0,
    0,
    0,
    '+',
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

void keyboard_isr(void)
{
    uint8_t scancode = in(KEYBOARD_DATA_PORT);

    if (keyboard_state.keyboard_input_on)
    {

        char ascii_char = keyboard_scancode_1_to_ascii_map[scancode];
        if (ascii_char != 0)
        {
            if (ascii_char == '\n')
            {
                line_lengths[keyboard_state.keyboard_row] = keyboard_state.keyboard_col;
                keyboard_state.keyboard_row++;
                keyboard_state.keyboard_col = 0;
            }
            else if (ascii_char == '\b')
            {
                if (keyboard_state.keyboard_col > 0)
                {
                    keyboard_state.keyboard_col--;
                    framebuffer_write(keyboard_state.keyboard_row, keyboard_state.keyboard_col, ' ', 0xF, 0);
                }
                else if (keyboard_state.keyboard_row > 0)
                {
                    keyboard_state.keyboard_row--;
                    keyboard_state.keyboard_col = line_lengths[keyboard_state.keyboard_row];
                    framebuffer_write(keyboard_state.keyboard_row, keyboard_state.keyboard_col, ' ', 0xF, 0);
                }
            }
            else
            {
                keyboard_state.print_mode = true;
                keyboard_state.keyboard_col++;
            }
            keyboard_state.keyboard_buffer = ascii_char;
        }
    }
    pic_ack(PIC1_OFFSET + IRQ_KEYBOARD);
}

void keyboard_state_activate(void)
{
    keyboard_state.keyboard_input_on = true;
}

void keyboard_state_deactivate(void)
{
    keyboard_state.keyboard_input_on = false;
}

void get_keyboard_buffer(char *buf, int *row, int *col, bool *print_mode)
{
    *buf = keyboard_state.keyboard_buffer;
    *row = keyboard_state.keyboard_row;
    *col = keyboard_state.keyboard_col;
    *print_mode = keyboard_state.print_mode;

    keyboard_state.keyboard_buffer = 0;
    keyboard_state.print_mode = false;
}