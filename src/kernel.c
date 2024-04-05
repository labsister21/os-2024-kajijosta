#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/cpu/keyboard.h"
#include <stdbool.h>

void kernel_setup(void)
{
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    keyboard_state_activate();
    while (true)
    {
        char c;
        int row, col;
        bool print_mode;
        get_keyboard_buffer(&c, &row, &col, &print_mode);

        if (c && print_mode)
        {
            col -= 1;
            framebuffer_write(row, col, c, 0xF, 0);
        }

        framebuffer_set_cursor(row, col);
    }
}
