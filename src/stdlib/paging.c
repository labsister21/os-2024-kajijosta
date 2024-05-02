#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/memory/paging.h"

__attribute__((aligned(0x1000))) struct PageDirectory _paging_kernel_page_directory = {
    .table = {
        [0] = {
            .flag.present_bit = 1,
            .flag.write_bit = 1,
            .flag.use_pagesize_4_mb = 1,
            .lower_address = 0,
        },
        [0x300] = {
            .flag.present_bit = 1,
            .flag.write_bit = 1,
            .flag.use_pagesize_4_mb = 1,
            .lower_address = 0,
        },
    }};

static struct PageManagerState page_manager_state = {
    .page_frame_map = {
        [0] = true,
        [1 ... PAGE_FRAME_MAX_COUNT - 1] = false},
    // TODO: Initialize page manager state properly
};

void update_page_directory_entry(
    struct PageDirectory *page_dir,
    void *physical_addr,
    void *virtual_addr,
    struct PageDirectoryEntryFlag flag)
{
    uint32_t page_index = ((uint32_t)virtual_addr >> 22) & 0x3FF;
    page_dir->table[page_index].flag = flag;
    page_dir->table[page_index].lower_address = ((uint32_t)physical_addr >> 22) & 0x3FF;
    flush_single_tlb(virtual_addr);
}

void flush_single_tlb(void *virtual_addr)
{
    asm volatile("invlpg (%0)" : /* <Empty> */ : "b"(virtual_addr) : "memory");
}

/* --- Memory Management --- */
// TODO: Implement
bool paging_allocate_check(uint32_t amount)
{
    // TODO: Check whether requested amount is available
    if (amount > PAGE_FRAME_MAX_COUNT)
    {
        return false; // Jumlah yang diminta melebihi batas maksimum
    }

    // Periksa apakah jumlah yang diminta tersedia
    for (uint32_t i = 0; i < amount; i++)
    {
        if (page_manager_state.page_frame_map[i] == true)
        {
            return false; // Frame halaman sudah digunakan
        }
    }
    return true;
}

bool paging_allocate_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr)
{
    /*
     * TODO: Find free physical frame and map virtual frame into it
     * - Find free physical frame in page_manager_state.page_frame_map[] using any strategies
     * - Mark page_manager_state.page_frame_map[]
     * - Update page directory with user flags:
     *     > present bit    true
     *     > write bit      true
     *     > user bit       true
     *     > pagesize 4 mb  true
     */
    if (page_dir == NULL || virtual_addr == NULL)
    {
        return false; // Parameter tidak valid
    }

    // Cari frame fisik yang kosong
    uint32_t free_frame_idx = 0;
    for (uint32_t i = 0; i < PAGE_FRAME_MAX_COUNT; i++)
    {
        if (page_manager_state.page_frame_map[i] == false)
        {
            free_frame_idx = i;
            page_manager_state.page_frame_map[i] = true; // Tandai frame sebagai digunakan
            break;
        }
    }

    // Periksa apakah ada frame fisik yang kosong
    if (free_frame_idx == 0 && page_manager_state.page_frame_map[0] == true)
    {
        return false; // Tidak ada frame fisik yang kosong
    }

    // Alokasikan frame halaman untuk pengguna
    struct PageDirectoryEntry *page_entry = &page_dir->table[free_frame_idx];
    page_entry->flag.present_bit = 1;
    page_entry->flag.write_bit = 1;
    page_entry->flag.user_bit = 1;
    page_entry->flag.use_pagesize_4_mb = 1; // Gunakan pagesize 4 MB
    // TODO: Update flags dan entry page directory sesuai kebutuhan
    return true;
}

bool paging_free_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr)
{
    /*
     * TODO: Deallocate a physical frame from respective virtual address
     * - Use the page_dir.table values to check mapped physical frame
     * - Remove the entry by setting it into 0
     */
    if (page_dir == NULL || virtual_addr == NULL)
    {
        return false; // Parameter tidak valid
    }

    // Cari entry page directory sesuai dengan alamat virtual
    uint32_t entry_idx = (uint32_t)virtual_addr / 4096; // Misalnya, gunakan ukuran halaman 4 KB
    if (entry_idx >= PAGE_FRAME_MAX_COUNT)
    {
        return false; // Alamat virtual melewati batas
    }

    // Periksa apakah entri page directory yang akan dihapus sudah ada
    struct PageDirectoryEntry *page_entry = &page_dir->table[entry_idx];
    if (page_entry->flag.present_bit == 0)
    {
        return false; // Tidak ada entri yang perlu dihapus
    }

    // Bebaskan frame halaman fisik
    page_manager_state.page_frame_map[entry_idx] = false;

    // Reset entri page directory
    page_entry->flag.present_bit = 0;
    page_entry->flag.write_bit = 0;
    page_entry->flag.user_bit = 0;
    page_entry->flag.use_pagesize_4_mb = 0;
    // TODO: Reset entri page directory sesuai kebutuhan
    return true;
}