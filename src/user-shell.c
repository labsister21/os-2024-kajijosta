#include <stdint.h>
#include "header/cpu/fat32.h"

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx)
{
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

int main(void)
{
    syscall(6, (uint32_t) "root@kajijOSta", 14, 0x2);
    syscall(5, (uint32_t)':', 0x8, 0);
    syscall(5, (uint32_t)'/', 0x1, 0);
    syscall(5, (uint32_t)'$', 0x8, 0);
    syscall(5, (uint32_t)' ', 0x8, 0);
    syscall(7, 0, 0, 0);
    while (true)
    {
        char buf;
        bool print_mode;
        syscall(4, (uint32_t)&buf, (uint32_t)&print_mode, 0);
        if (buf && print_mode)
        {
            syscall(6, (uint32_t)&buf, 1, 0xF);
        }
    }

    return 0;
}
