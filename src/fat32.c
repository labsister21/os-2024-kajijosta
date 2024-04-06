#include "header/cpu/fat32.h"
#include "header/stdlib/string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C',
    'o',
    'u',
    'r',
    's',
    'e',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    'D',
    'e',
    's',
    'i',
    'g',
    'n',
    'e',
    'd',
    ' ',
    'b',
    'y',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    'L',
    'a',
    'b',
    ' ',
    'S',
    'i',
    's',
    't',
    'e',
    'r',
    ' ',
    'I',
    'T',
    'B',
    ' ',
    ' ',
    'M',
    'a',
    'd',
    'e',
    ' ',
    'w',
    'i',
    't',
    'h',
    ' ',
    '<',
    '3',
    ' ',
    ' ',
    ' ',
    ' ',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '2',
    '0',
    '2',
    '4',
    '\n',
    [BLOCK_SIZE - 2] = 'O',
    [BLOCK_SIZE - 1] = 'k',
};

static struct FAT32DriverState fat32_driver_state;

void create_fat32(void)
{
  // Write file system signature into boot sector
  write_blocks(fs_signature, BOOT_SECTOR, 1);

  // Initialize File Allocation Table with reserved values
  struct FAT32FileAllocationTable *fat = &fat32_driver_state.fat_table;
  fat->cluster_map[0] = CLUSTER_0_VALUE;
  fat->cluster_map[1] = CLUSTER_1_VALUE;
  for (int i = 2; i < CLUSTER_MAP_SIZE; i++)
  {
    fat->cluster_map[i] = 0; // Unused clusters are initialized to 0
  }

  // Write File Allocation Table to the disk
  write_clusters(fat, FAT_CLUSTER_NUMBER, 1);

  // Initialize root directory
  struct FAT32DirectoryTable *dir = &fat32_driver_state.dir_table_buf;
  init_directory_table(dir, "ROOT", ROOT_CLUSTER_NUMBER);

  // Write root directory to the disk
  write_clusters(dir, ROOT_CLUSTER_NUMBER, 1);
}

bool is_empty_storage(void)
{
  uint8_t boot_sector[BLOCK_SIZE];
  read_blocks(boot_sector, BOOT_SECTOR, 1);
  return memcmp(boot_sector, fs_signature, BLOCK_SIZE);
}

void initialize_filesystem_fat32(void)
{
  if (is_empty_storage())
  {
    create_fat32();
  }
  else
  {
    read_clusters(&fat32_driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
  }
}

uint32_t cluster_to_lba(uint32_t cluster) { return cluster * CLUSTER_BLOCK_COUNT; }

void write_clusters(const void *ptr, uint32_t cluster_number,
                    uint8_t cluster_count)
{
  uint32_t block_number = cluster_to_lba(cluster_number);

  uint8_t block_count = cluster_to_lba(cluster_count);

  write_blocks(ptr, block_number, block_count);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count)
{
  uint32_t block_number = cluster_to_lba(cluster_number);

  uint8_t block_count = cluster_to_lba(cluster_count);

  read_blocks(ptr, block_number, block_count);
}
void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name,
                          uint32_t parent_dir_cluster)
{
  // Initialize all entries in the directory table to 0
  memset(dir_table, 0, sizeof(struct FAT32DirectoryTable));

  // Create a new directory entry for the parent directory
  struct FAT32DirectoryEntry *entry = &dir_table->table[0];
  memcpy(entry->name, name, sizeof(entry->name));
  entry->attribute = ATTR_SUBDIRECTORY;
  entry->cluster_low = parent_dir_cluster & 0xFFFF;
  entry->cluster_high = (parent_dir_cluster >> 16) & 0xFFFF;
}

int8_t read(struct FAT32DriverRequest request)
{

  struct FAT32DirectoryTable dir;
  read_clusters(&dir, request.parent_cluster_number, 1);

  // Mencari file dalam direktori
  for (uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
  {
    struct FAT32DirectoryEntry entry = dir.table[i];

    // Memeriksa apakah entry memiliki nama yang sama dengan request
    if (memcmp(entry.name, request.name, sizeof(entry.name)) == 0 &&
        memcmp(entry.ext, request.ext, 3) == 0)
    {
      // Jika entry bukan file
      if (entry.attribute & ATTR_SUBDIRECTORY)
      {
        return 1;
      }

      // Memeriksa apakah buffer cukup besar untuk menyimpan file
      if (request.buffer_size < entry.filesize)
      {
        return -1;
      }

      // Membaca file dan menyimpan ke buffer
      uint32_t cluster_number = (entry.cluster_high << 16) | entry.cluster_low;
      uint8_t i = 0;
      while (cluster_number != FAT32_FAT_END_OF_FILE)
      {
        read_clusters(request.buf, cluster_number, 1);
        request.buf += CLUSTER_SIZE * i;
        cluster_number =
            fat32_driver_state.fat_table.cluster_map[cluster_number];
        i++;
      }

      return 0;
    }
  }

  return 2; // Error: file tidak ditemukan
}

int8_t read_directory(struct FAT32DriverRequest request)
{
  // Membaca direktori
  struct FAT32DirectoryTable dir;
  read_clusters(&dir, request.parent_cluster_number, 1);

  // Mencari folder dalam direktori
  for (uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry);
       i++)
  {
    struct FAT32DirectoryEntry entry = dir.table[i];

    // Memeriksa apakah entry memiliki nama yang sama dengan request
    if (memcmp(entry.name, request.name, sizeof(entry.name)) == 0)
    {
      // Jika entry adalah file, kembalikan error
      if (!(entry.attribute & ATTR_SUBDIRECTORY))
      {
        return 1; // Error: Bukan sebuah folder
      }

      // Memeriksa apakah buffer cukup besar untuk menyimpan folder
      if (request.buffer_size < sizeof(struct FAT32DirectoryTable))
      {
        return -1; // Error: buffer tidak cukup besar
      }

      // Membaca folder dan menyimpan ke buffer
      uint32_t cluster_number = (entry.cluster_high << 16) | entry.cluster_low;
      read_clusters(request.buf, cluster_number, 1);

      return 0;
    }
  }

  return 2; // Error: folder tidak ditemukan
}

// TODO masi salah
int8_t write(struct FAT32DriverRequest request)
{
  // Membaca direktori
  read_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);

  // Mencari entry kosong dalam direktori
  uint8_t i;
  for (i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
  {
    if (fat32_driver_state.dir_table_buf.table[i].name[0] == 0)
    {
      break;
    }
  }

  // Jika direktori penuh
  if (i == CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry))
  {
    return 1; // Error: direktori penuh
  }

  // Membuat entry baru
  struct FAT32DirectoryEntry *entry = &fat32_driver_state.dir_table_buf.table[i];
  memcpy(entry->name, request.name, sizeof(entry->name));
  memcpy(entry->ext, request.ext, sizeof(entry->ext));
  entry->attribute = 0;
  entry->cluster_low = 0;
  entry->cluster_high = 0;
  entry->filesize = request.buffer_size;

  // Jika request.buffer_size = 0, buat direktori
  if (request.buffer_size == 0)
  {
    entry->attribute = ATTR_SUBDIRECTORY;
  }
  else
  {
    entry->attribute = 0;
  }

  // Mencari cluster kosong
  uint32_t cluster_number = 2;
  while (fat32_driver_state.fat_table.cluster_map[cluster_number] != 0)
  {
    cluster_number++;
  }

  // Menulis file ke cluster
  uint32_t remaining_size = request.buffer_size;
  uint32_t current_cluster = cluster_number;
  while (remaining_size > 0)
  {
    uint32_t write_size = remaining_size > CLUSTER_SIZE ? CLUSTER_SIZE : remaining_size;
    memcpy(fat32_driver_state.cluster_buf.buf, request.buf, write_size);
    write_clusters(&fat32_driver_state.cluster_buf, current_cluster, 1);

    remaining_size -= write_size;
    request.buf += write_size;

    if (remaining_size > 0)
    {
      fat32_driver_state.fat_table.cluster_map[current_cluster] = current_cluster + 1;
      current_cluster++;
    }
    else
    {
      fat32_driver_state.fat_table.cluster_map[current_cluster] = FAT32_FAT_END_OF_FILE;
    }
  }

  // Menyimpan File Allocation Table
  write_clusters(&fat32_driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
  return 0;
}

// TODO masi salah
int8_t delete(struct FAT32DriverRequest request)
{
  // Membaca direktori
  read_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);

  for (uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
  {
    struct FAT32DirectoryEntry entry = fat32_driver_state.dir_table_buf.table[i];

    // Memeriksa apakah entry memiliki nama yang sama dengan request
    if (memcmp(entry.name, request.name, sizeof(entry.name)) == 0 &&
        memcmp(entry.ext, request.ext, sizeof(entry.ext)) == 0)
    {
      // Menghapus file
      uint32_t cluster_number = (entry.cluster_high << 16) | entry.cluster_low;
      while (cluster_number != FAT32_FAT_END_OF_FILE)
      {
        uint32_t next_cluster_number = fat32_driver_state.fat_table.cluster_map[cluster_number];
        fat32_driver_state.fat_table.cluster_map[cluster_number] = 0;
        cluster_number = next_cluster_number;
      }

      // Menghapus entry
      memset(&fat32_driver_state.dir_table_buf.table[i], 0, sizeof(struct FAT32DirectoryEntry));
      write_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);

      // Menyimpan File Allocation Table
      write_clusters(&fat32_driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);

      return 0;
    }
  }

  return 1; // Error: file tidak ditemukan
}