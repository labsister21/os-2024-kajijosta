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

// Test shell
void kernel_setup(void)
{
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    gdt_install_tss();
    set_tss_register();

    // Allocate first 4 MiB virtual memory
    paging_allocate_user_page_frame(&_paging_kernel_page_directory, (uint8_t *)0);

    // Write shell into memory
    struct FAT32DriverRequest request = {
        .buf = (uint8_t *)0,
        .name = "shell",
        .ext = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size = 0x100000,
    };
    read(request);

    // Set TSS $esp pointer and jump into shell
    set_tss_kernel_current_stack();
    kernel_execute_user_program((uint8_t *)0);

    while (true)
        ;
}

// Using keyboard
// void kernel_setup(void)
// {
//     load_gdt(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     activate_keyboard_interrupt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     keyboard_state_activate();
//     puts("root@kajijOSta", 15, 0x2);
//     puts(":", 2, 0x8);
//     puts("/", 2, 0x1);
//     puts("$", 2, 0x8);
//     while (true)
//     {
//         char c;
//         bool print_mode;
//         get_keyboard_buffer(&c, &print_mode);
//         if (c && print_mode)
//         {
//             puts(&c, 1, 0xF);
//         }
//     }
// }

// Test user shell
// void kernel_setup(void) {
//     load_gdt(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     activate_keyboard_interrupt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     initialize_filesystem_fat32();
//     gdt_install_tss();
//     set_tss_register();

//     // Allocate first 4 MiB virtual memory
//     paging_allocate_user_page_frame(&_paging_kernel_page_directory, (uint8_t*) 0);

//     // Write shell into memory
//     struct FAT32DriverRequest request = {
//         .buf                   = (uint8_t*) 0,
//         .name                  = "shell",
//         .ext                   = "\0\0\0",
//         .parent_cluster_number = ROOT_CLUSTER_NUMBER,
//         .buffer_size           = 0x100000,
//     };
//     read(request);

//     // Set TSS $esp pointer and jump into shell
//     set_tss_kernel_current_stack();
//     kernel_execute_user_program((uint8_t*) 0);

//     while (true);
// }

// // // Test CRUD
// void kernel_setup(void)
// {
//     load_gdt(&_gdt_gdtr);
//     pic_remap();
//     activate_keyboard_interrupt();
//     initialize_idt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     initialize_filesystem_fat32();
//     // struct BlockBuffer b;
//     // for (int i = 0; i < 512; i++)
//     //     b.buf[i] = i % 16;
//     // write_blocks(&b, 17, 1);
//     // struct BlockBuffer b;
//     // read_blocks(&b, 17, 1);
//     struct ClusterBuffer buf;
//     struct FAT32DriverRequest req2 = {.buf = &buf,
//                                       .name = "shell",
//                                       .parent_cluster_number = 0x02,
//                                       .buffer_size = 651};
//     uint8_t err = write(req2);
//     if (err == 0)
//         puts("0", 2, 0xF);
//     else if (err == 1)
//         puts("1", 2, 0xF);
//     else if (err == 2)
//         puts("2", 2, 0xF);
//     else
//     {
//         puts("-1", 3, 0xF);
//     }

//     while (true)
//         ;
//     // Write file
// }