#include "header/cpu/disk.h"
#include "header/cpu/portio.h"

// Menunggu hingga disk tidak sibuk
static void ATA_busy_wait()
{
    while (in(0x1F7) & ATA_STATUS_BSY)
        ;
}

// Menunggu sampai disk siap untuk transfer data
static void ATA_DRQ_wait()
{
    while (!(in(0x1F7) & ATA_STATUS_RDY))
        ;
}

// Membaca blok dari disk
void read_blocks(void *ptr, uint32_t logical_block_address, uint8_t block_count)
{
    ATA_busy_wait();
    out(0x1F6, 0xE0 | ((logical_block_address >> 24) & 0xF)); // Mengirimkan 4 bit tinggi dari LBA ke port drive/head
    out(0x1F2, block_count); // Mengirimkan jumlah blok yang akan dibaca ke port count
    out(0x1F3, (uint8_t)logical_block_address); // Mengirimkan 8 bit terendah dari LBA ke port LBAlo
    out(0x1F4, (uint8_t)(logical_block_address >> 8)); // Mengirimkan 8 bit berikutnya dari LBA ke port LBAmid
    out(0x1F5, (uint8_t)(logical_block_address >> 16)); // Mengirimkan 8 bit berikutnya dari LBA ke port LBAhi
    out(0x1F7, 0x20);  // Mengirimkan perintah baca ke port command/status

    uint16_t *target = (uint16_t *)ptr;
    for (uint32_t i = 0; i < block_count; i++)
    {
        ATA_busy_wait();
        ATA_DRQ_wait();
        for (uint32_t j = 0; j < HALF_BLOCK_SIZE; j++)
            target[j] = in16(0x1F0);
        target += HALF_BLOCK_SIZE;
    }
}

// Menulis blok data ke disk
void write_blocks(const void *ptr, uint32_t logical_block_address, uint8_t block_count)
{
    ATA_busy_wait();
    out(0x1F6, 0xE0 | ((logical_block_address >> 24) & 0xF));  // Mengirimkan 4 bit tinggi dari LBA ke port drive/head
    out(0x1F2, block_count); // Mengirimkan jumlah blok yang akan ditulis ke port count
    out(0x1F3, (uint8_t)logical_block_address); // Mengirimkan 8 bit terendah dari LBA ke port LBAlo
    out(0x1F4, (uint8_t)(logical_block_address >> 8));  // Mengirimkan 8 bit berikutnya dari LBA ke port LBAmid
    out(0x1F5, (uint8_t)(logical_block_address >> 16)); // Mengirimkan 8 bit berikutnya dari LBA ke port LBAhi
    out(0x1F7, 0x30);  // Mengirimkan perintah tulis ke port command/status

    for (uint32_t i = 0; i < block_count; i++)
    {
        ATA_busy_wait();
        ATA_DRQ_wait();
        for (uint32_t j = 0; j < HALF_BLOCK_SIZE; j++)
            out16(0x1F0, ((uint16_t *)ptr)[HALF_BLOCK_SIZE * i + j]);
    }
}
