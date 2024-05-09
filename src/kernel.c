#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/cpu/keyboard.h"
#include "header/cpu/fat32.h"
#include "header/cpu/disk.h"
#include "header/cpu/paging.h"
#include <stdbool.h>


void kernel_setup(void)
{
    // Test paging
    paging_allocate_user_page_frame(&_paging_kernel_page_directory, (uint8_t*) 0x600000);
    *((uint8_t*) 0x500000) = 1;
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

// void kernel_setup(void)
// {
//   load_gdt(&_gdt_gdtr);
//   pic_remap();
//   activate_keyboard_interrupt();
//   initialize_idt();
//   framebuffer_clear();
//   framebuffer_set_cursor(0, 0);
//   initialize_filesystem_fat32();
//   // struct BlockBuffer b;
//   // for (int i = 0; i < 512; i++)
//   //     b.buf[i] = i % 16;
//   // write_blocks(&b, 17, 1);
//   // struct BlockBuffer b;
//   // read_blocks(&b, 17, 1);
//   struct ClusterBuffer buf;
//   struct FAT32DriverRequest req2 = {.buf = &buf,
//                                     .name = "kano",
//                                     .parent_cluster_number = 0x02,
//                                     .buffer_size = 10000};
//   uint8_t err = delete(req2);
//   if (err == 0)
//     framebuffer_write(0, 0, '0', 0xF, 0);
//   else if (err == 1)
//     framebuffer_write(0, 0, '1', 0xF, 0);
//   else if (err == 2)
//     framebuffer_write(0, 0, '2', 0xF, 0);
//   else
//   {
//     framebuffer_write(0, 0, '-', 0xF, 0);
//     framebuffer_write(0, 1, '1', 0xF, 0);
//   }

//   while (true)
//     ;
//   // Write file
// }