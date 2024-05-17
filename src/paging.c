#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/cpu/paging.h"

__attribute__((aligned(0x1000))) struct PageDirectory _paging_kernel_page_directory = {
    .table = {
        [0] = {
            .flag.present_bit       = 1,
            .flag.write_bit         = 1,
            .flag.use_pagesize_4_mb = 1,
            .lower_address          = 0,
        },
        [0x300] = {
            .flag.present_bit       = 1,
            .flag.write_bit         = 1,
            .flag.use_pagesize_4_mb = 1,
            .lower_address          = 0,
        },
    }
};

static struct PageManagerState page_manager_state = {
    .page_frame_map = {
        [0]                            = true,
        [1 ... PAGE_FRAME_MAX_COUNT-1] = false
    },
    .free_page_frame_count = PAGE_FRAME_MAX_COUNT - 1, // All frames except the first one are free
};

void update_page_directory_entry(
    struct PageDirectory *page_dir,
    void *physical_addr, 
    void *virtual_addr, 
    struct PageDirectoryEntryFlag flag
) {
    uint32_t page_index = ((uint32_t) virtual_addr >> 22) & 0x3FF;
    page_dir->table[page_index].flag          = flag;
    page_dir->table[page_index].lower_address = ((uint32_t) physical_addr >> 22) & 0x3FF;
    flush_single_tlb(virtual_addr);
}

void flush_single_tlb(void *virtual_addr) {
    asm volatile("invlpg (%0)" : /* <Empty> */ : "b"(virtual_addr): "memory");
}


/* --- Memory Management --- */
bool paging_allocate_check(uint32_t amount) {
    // Calculate the number of page frames required for the requested amount
    return amount <= PAGE_FRAME_SIZE * PAGE_FRAME_MAX_COUNT;
}


bool paging_allocate_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr) {
    // Find a free physical frame
    // Find a free page frame
    for (uint32_t i = 0; i < PAGE_FRAME_MAX_COUNT; i++)
    {
        if (!page_manager_state.page_frame_map[i])
        {
            // Mark the page frame as used
            page_manager_state.page_frame_map[i] = true;
            page_manager_state.free_page_frame_count--;

            // Update the page directory
            struct PageDirectoryEntryFlag flag = {1, 1, 1, 0, 0, 0, 0, 1};
            update_page_directory_entry(page_dir, (void *)(i * PAGE_FRAME_SIZE), virtual_addr, flag);

            // Invalidate the TLB entry for the virtual address
            flush_single_tlb(virtual_addr);

            return true;
        }
    }

    // No free page frames
    return false;
}

bool paging_free_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr) {
   uint32_t index = (uint32_t)virtual_addr / PAGE_FRAME_SIZE;

    // Check if the page frame is used
    if (!page_manager_state.page_frame_map[index])
    {
        return false;
    }

    // Mark the page frame as free
    page_manager_state.page_frame_map[index] = false;
    page_manager_state.free_page_frame_count++;

    // Clear the page directory entry
    struct PageDirectoryEntryFlag flag = {0, 0, 0, 0, 0, 0, 0, 0};
    update_page_directory_entry(page_dir, (void *)(index * PAGE_FRAME_SIZE), virtual_addr, flag);

    // Invalidate the TLB entry for the virtual address
    flush_single_tlb(virtual_addr);

    return true;
}

// Simulated physical-to-virtual address mapping offset
#define KERNEL_VIRTUAL_ADDRESS_BASE 0xC0000000

// Array to simulate physical page directory storage
__attribute__((aligned(0x1000))) static struct PageDirectory page_directory_pool[PAGING_DIRECTORY_TABLE_MAX_COUNT];
static bool page_directory_usage[PAGING_DIRECTORY_TABLE_MAX_COUNT] = {false};

struct PageDirectory* paging_create_new_page_directory(void) {
    for (int i = 0; i < PAGING_DIRECTORY_TABLE_MAX_COUNT; i++) {
        if (!page_directory_usage[i]) {
            page_directory_usage[i] = true;
            struct PageDirectory* pd = &page_directory_pool[i];

            // Zero out the new page directory
            memset(pd, 0, sizeof(struct PageDirectory));

            // Setup default kernel mapping as an example
            pd->table[0x300] = (struct PageDirectoryEntry){
                .present = 1,
                .writable = 1,
                .pageSize = 1,
                .address = 0 // Map to 0 physical address
            };
            return pd;
        }
    }
    return NULL; // No available page directories
}

bool paging_free_page_directory(struct PageDirectory *page_dir) {
    for (int i = 0; i < PAGING_DIRECTORY_TABLE_MAX_COUNT; i++) {
        if (page_dir == &page_directory_pool[i] && page_directory_usage[i]) {
            memset(page_dir, 0, sizeof(struct PageDirectory));
            page_directory_usage[i] = false;
            return true;
        }
    }
    return false; // Page directory not found or already free
}

struct PageDirectory* paging_get_current_page_directory_addr(void) {
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return (struct PageDirectory *)(cr3 + KERNEL_VIRTUAL_ADDRESS_BASE);
}

void paging_use_page_directory(struct PageDirectory *page_dir_virtual_addr) {
    uint32_t phys_addr = (uint32_t)page_dir_virtual_addr - KERNEL_VIRTUAL_ADDRESS_BASE;
    __asm__ volatile("mov %0, %%cr3" : : "r"(phys_addr) : "memory");
}