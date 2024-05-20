#include "header/cpu/fat32.h"
#include "header/stdlib/string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// File system signature
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

// Fungsi untuk membuat FAT32 file system
void create_fat32(void)
{
  // Menulis file system signature ke boot sector
  write_blocks(fs_signature, BOOT_SECTOR, 1);

  // Menginsialisasi File Allocation Table dengan reserved values
  struct FAT32FileAllocationTable *fat = &fat32_driver_state.fat_table;
  fat->cluster_map[0] = CLUSTER_0_VALUE;
  fat->cluster_map[1] = CLUSTER_1_VALUE;
  for (int i = 2; i < CLUSTER_MAP_SIZE; i++)
  {
    fat->cluster_map[i] = 0; // Clusters yang tidak digunakan diinisialisasi ke 0
  }

  // Menulis File Allocation Table ke disk
  write_clusters(fat, FAT_CLUSTER_NUMBER, 1);

  // Menginisialisasi root directory
  struct FAT32DirectoryTable *dir = &fat32_driver_state.dir_table_buf;
  init_directory_table(dir, "ROOT", ROOT_CLUSTER_NUMBER);

  // Menulis root directory ke disk
  write_clusters(dir, ROOT_CLUSTER_NUMBER, 1);
}

// Fungsi untuk mengecek isi storage
bool is_empty_storage(void)
{
  uint8_t boot_sector[BLOCK_SIZE];
  read_blocks(boot_sector, BOOT_SECTOR, 1);
  return memcmp(boot_sector, fs_signature, BLOCK_SIZE);
}

// Menginisialisasi FAT32 File System
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

// Mengonversi cluster number ke LBA
uint32_t cluster_to_lba(uint32_t cluster) { return cluster * CLUSTER_BLOCK_COUNT; }

// Menulis clusters ke disk
void write_clusters(const void *ptr, uint32_t cluster_number,
                    uint8_t cluster_count)
{
  uint32_t block_number = cluster_to_lba(cluster_number);

  uint8_t block_count = cluster_to_lba(cluster_count);

  write_blocks(ptr, block_number, block_count);
}

// Membaca clusters dari disk
void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count)
{
  uint32_t block_number = cluster_to_lba(cluster_number);

  uint8_t block_count = cluster_to_lba(cluster_count);

  read_blocks(ptr, block_number, block_count);
}

// Menginisialisasi Directory Table
void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name,
                          uint32_t parent_dir_cluster)
{
  // Menginisialisasi semua entries di directory table dengan 0
  memset(dir_table, 0, sizeof(struct FAT32DirectoryTable));

  // Membuat directory entry baru untuk parent directory
  struct FAT32DirectoryEntry *entry = &dir_table->table[0];
  memcpy(entry->name, name, sizeof(entry->name));
  entry->attribute = ATTR_SUBDIRECTORY;
  entry->cluster_low = parent_dir_cluster & 0xFFFF;
  entry->cluster_high = (parent_dir_cluster >> 16) & 0xFFFF;
}

// Membulatkan floating-point number ke bilangan integer berikutnya
uint8_t ceil(float n)
{
  if ((int)n == n)
    return n;
  return (int)n + 1;
}

// Membaca file dari FAT32 file system
int8_t read(struct FAT32DriverRequest request)
{
  // Mengecek jika parent cluster number bukan end of file marker
  if (fat32_driver_state.fat_table.cluster_map[request.parent_cluster_number] != FAT32_FAT_END_OF_FILE)
  {
    return 2;
  }

  // Menginisalisasi struktur sebuah directory table
  struct FAT32DirectoryTable dir_table;
  memset(&dir_table, 0, CLUSTER_SIZE);

  // Membaca directory table dari disk
  read_clusters(&dir_table, request.parent_cluster_number, 1);

  for (uint8_t i = 0; i < 64; i++)
  {
    struct FAT32DirectoryEntry dir_entry = dir_table.table[i];

    // Mengecek apakah entry sesuai dengan nama dan ekstensi requested file
    if (!memcmp(&dir_entry.name, &request.name, 8) && !memcmp(&dir_entry.ext, &request.ext, 3))
    {
      // Mengecek apakah entry merupakan directory
      if (dir_entry.attribute == 0x10)
      {
        return 1;
      }

      // Mengecek apakah kapasitas buffer size cukup untuk menyimpan file data
      if ((request.buffer_size / CLUSTER_SIZE) < ceil(dir_entry.filesize / (float)CLUSTER_SIZE))
      {
        return -1;
      }

      // Membaca file data clusters dari disk
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

// Membaca directory dari the FAT32 file system
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
  // Mengecek apakah  parent cluster number bukan end of file marker
  if (fat32_driver_state.fat_table.cluster_map[request.parent_cluster_number] != FAT32_FAT_END_OF_FILE)
    return 2;

  // Menginisaliasi struktur dari directory table
  struct FAT32DirectoryTable dir_table;
  memset(&dir_table, 0, CLUSTER_SIZE);

  // Membaca directory table dari disk
  read_clusters(&dir_table, request.parent_cluster_number, 1);

  uint8_t dirtable_empty_slot = 0;

  for (uint8_t i = 2; i < 64; i++)
  {
    struct FAT32DirectoryEntry dir_entry = dir_table.table[i];

    // Mencari slot kosong di directory table
    if (dir_entry.user_attribute != UATTR_NOT_EMPTY && dirtable_empty_slot == 0)
    {
      dirtable_empty_slot = i;
    }

    // Mengecek jika entry sesuai dengan nama dan ekstensi requested file
    if (!memcmp(&dir_entry.name, &request.name, 8) && !memcmp(&dir_entry.ext, &request.ext, 3))
      return 1; // Error: File sudah ada
  }

  if (dirtable_empty_slot == 0)
    return -1; // Error: Directory table penuh

  uint8_t required = ceil(request.buffer_size / (float)CLUSTER_SIZE);

  bool isFolder = false;
  if (request.buffer_size == 0)
  {
    isFolder = true;
    required += 1; // Untuk folder, dibutuhkan satu cluster
  }

  uint32_t slot_buf[required];
  uint8_t empty = 0;
  uint32_t i = 0;

  // Mencari slots kosong di cluster map untuk file data
  while (empty < required && i < CLUSTER_MAP_SIZE)
  {
    if (fat32_driver_state.fat_table.cluster_map[i] == FAT32_FAT_EMPTY_ENTRY)
    {
      slot_buf[empty++] = i;
    }
    i++;
  }

  if (empty < required)
    return -1; // Error: Empty clusters tidak cukup untuk file data

  // Jika folder, insialisasi directory tablenya
  if (isFolder)
  {
    struct FAT32DirectoryTable child_dir;
    memset(&child_dir, 0, CLUSTER_SIZE);
    init_directory_table(&child_dir, request.name, request.parent_cluster_number);

    struct FAT32DirectoryEntry *child = &child_dir.table[0];
    child->cluster_low = slot_buf[0] & 0xFFFF;
    child->cluster_high = (slot_buf[0] >> 16) & 0xFFFF;

    // Menulis directory table ke disk
    write_clusters(&child_dir, slot_buf[0], 1);

    // Menandai last cluster di folder sebagai EOF
    fat32_driver_state.fat_table.cluster_map[slot_buf[0]] = FAT32_FAT_END_OF_FILE;
    write_clusters(&(fat32_driver_state.fat_table), FAT_CLUSTER_NUMBER, 1);
  }
  else // Jika file
  {
    // Menulis file data clusters ke disk
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

  // Membuat sebuah entry untuk file atau folder di directory table
  struct FAT32DirectoryEntry entry = {
      .attribute = isFolder ? ATTR_SUBDIRECTORY : 0,
      .user_attribute = UATTR_NOT_EMPTY,

      .cluster_low = slot_buf[0] & 0xFFFF,
      .cluster_high = (slot_buf[0] >> 16) & 0xFFFF,
      .filesize = request.buffer_size};

  // Menyalin nama dan ekstensi file atau folder ke entry
  for (uint8_t a = 0; a < 8; a++)
    entry.name[a] = request.name[a];
  for (uint8_t b = 0; b < 3; b++)
    entry.ext[b] = request.ext[b];

  // Menambahkan entry ke directory table
  dir_table.table[dirtable_empty_slot] = entry;

  // Menulis directory table yang telah diperbarui ke disk
  write_clusters(&dir_table, request.parent_cluster_number, 1);

  return 0;
}

// Menghapus file dari FAT32 file system
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