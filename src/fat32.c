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

uint8_t ceil(float n)
{
  if ((int)n == n)
    return n;
  return (int)n + 1;
}

int8_t read(struct FAT32DriverRequest request)
{

  if (fat32_driver_state.fat_table.cluster_map[request.parent_cluster_number] != FAT32_FAT_END_OF_FILE)
  {
    return 2;
  }

  struct FAT32DirectoryTable dir_table;
  memset(&dir_table, 0, CLUSTER_SIZE);
  read_clusters(&dir_table, request.parent_cluster_number, 1);

  for (uint8_t i = 0; i < 64; i++)
  {
    struct FAT32DirectoryEntry dir_entry = dir_table.table[i];

    if (!memcmp(&dir_entry.name, &request.name, 8) && !memcmp(&dir_entry.ext, &request.ext, 3))
    {

      if (dir_entry.attribute == 0x10)
      {
        return 1;
      }

      if ((request.buffer_size / CLUSTER_SIZE) < ceil(dir_entry.filesize / (float)CLUSTER_SIZE))
      {
        return -1;
      }

      uint32_t current_cluster = (dir_entry.cluster_high << 16) | dir_entry.cluster_low;
      uint32_t clusters_read = 0;
      while (current_cluster != FAT32_FAT_END_OF_FILE)
      {
        read_clusters(request.buf + clusters_read * CLUSTER_SIZE, current_cluster, 1);
        current_cluster = fat32_driver_state.fat_table.cluster_map[current_cluster];
        clusters_read++;
      }
      return 0;
    }
  }

  return 2;
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
        return 1; // Error: Bukan sebuah folder (file)
      }

      // Error jika buffer size < filesize
      if (request.buffer_size < entry.filesize)
      {
        return -1; // Error: Buffer tidak cukup besar
      }

      // Membaca folder dan menyimpan ke buffer
      uint32_t cluster_number = (entry.cluster_high << 16) | entry.cluster_low;
      read_clusters(request.buf, cluster_number, 1);

      return 0;
    }
  }

  return 2; // Error: folder tidak ditemukan
}

// DONE
int8_t write(struct FAT32DriverRequest request)
{
  if (fat32_driver_state.fat_table.cluster_map[request.parent_cluster_number] != FAT32_FAT_END_OF_FILE)
    return 2;

  struct FAT32DirectoryTable dir_table;
  memset(&dir_table, 0, CLUSTER_SIZE);
  read_clusters(&dir_table, request.parent_cluster_number, 1);

  uint8_t dirtable_empty_slot = 0;

  for (uint8_t i = 0; i < 64; i++)
  {
    struct FAT32DirectoryEntry dir_entry = dir_table.table[i];

    if (dir_entry.user_attribute != UATTR_NOT_EMPTY && dirtable_empty_slot == 0)
    {
      dirtable_empty_slot = i;
    }

    if (!memcmp(&dir_entry.name, &request.name, 8) && !memcmp(&dir_entry.ext, &request.ext, 3))
      return 1;
  }

  if (dirtable_empty_slot == 0)
    return -1;

  uint8_t required = ceil(request.buffer_size / (float)CLUSTER_SIZE);

  bool isFolder = false;
  if (request.buffer_size == 0)
  {
    isFolder = true;
    required += 1;
  }

  uint32_t slot_buf[required];
  uint8_t empty = 0;
  uint32_t i = 0;

  while (empty < required && i < CLUSTER_MAP_SIZE)
  {
    if (fat32_driver_state.fat_table.cluster_map[i] == FAT32_FAT_EMPTY_ENTRY)
    {
      slot_buf[empty++] = i;
    }
    i++;
  }

  if (empty < required)
    return -1;

  if (isFolder)
  {
    struct FAT32DirectoryTable child_dir;
    memset(&child_dir, 0, CLUSTER_SIZE);
    init_directory_table(&child_dir, request.name, request.parent_cluster_number);

    struct FAT32DirectoryEntry *child = &child_dir.table[0];
    child->cluster_low = slot_buf[0] & 0xFFFF;
    child->cluster_high = (slot_buf[0] >> 16) & 0xFFFF;

    write_clusters(&child_dir, slot_buf[0], 1);

    fat32_driver_state.fat_table.cluster_map[slot_buf[0]] = FAT32_FAT_END_OF_FILE;
    write_clusters(&(fat32_driver_state.fat_table), FAT_CLUSTER_NUMBER, 1);
  }
  else
  {

    for (uint8_t j = 0; j < required; j++)
    {

      uint32_t cluster_num = slot_buf[j];
      uint32_t next_cluster = (j < required - 1) ? slot_buf[j + 1] : FAT32_FAT_END_OF_FILE;

      fat32_driver_state.fat_table.cluster_map[cluster_num] = next_cluster;
      write_clusters(&(fat32_driver_state.fat_table), FAT_CLUSTER_NUMBER, 1);

      write_clusters(request.buf, cluster_num, 1);
      request.buf += CLUSTER_SIZE;
    }
  }

  struct FAT32DirectoryEntry entry = {
      .attribute = isFolder ? ATTR_SUBDIRECTORY : 0,
      .user_attribute = UATTR_NOT_EMPTY,

      .cluster_low = slot_buf[0] & 0xFFFF,
      .cluster_high = (slot_buf[0] >> 16) & 0xFFFF,
      .filesize = request.buffer_size};

  for (uint8_t a = 0; a < 8; a++)
    entry.name[a] = request.name[a];
  for (uint8_t b = 0; b < 3; b++)
    entry.ext[b] = request.ext[b];

  dir_table.table[dirtable_empty_slot] = entry;
  write_clusters(&dir_table, request.parent_cluster_number, 1);

  return 0;
}

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