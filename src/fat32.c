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
      // Jika entry bukan file (directory)
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

double ceil(double x)
{
  int intPart = (int)x;
  return (x > intPart) ? (double)(intPart + 1) : (double)intPart;
}

// DONE
int8_t write(struct FAT32DriverRequest request)
{
  // Membaca direktori
  read_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);

  // Mencari entry kosong dalam direktori
  uint8_t idx;
  for (idx = 0; idx < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); idx++)
  {
    if (fat32_driver_state.dir_table_buf.table[idx].name[0] == 0)
    {
      break;
    }
  }

  // Jika direktori invalid
  if (fat32_driver_state.dir_table_buf.table[0].user_attribute != UATTR_NOT_EMPTY)
  {
    return 2; // Error: direktori tidak valid
  }

  // Cek jika ada nama file atau folder yang sama
  for (uint8_t j = 0; j < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); j++)
  {
    struct FAT32DirectoryEntry entry = fat32_driver_state.dir_table_buf.table[j];
    if (memcmp(entry.name, request.name, sizeof(entry.name)) == 0 &&
        memcmp(entry.ext, request.ext, sizeof(entry.ext)) == 0)
    {
      return 1; // Error: file/folder dengan nama yang sama sudah ada
    }
  }

  if (request.buffer_size == 0)
  {
    // folder
    // membuat sub-direktori pada folder parent request.parent_cluster_number dengan nama request.name
    struct FAT32DirectoryEntry newEntry =
        {
            .attribute = ATTR_SUBDIRECTORY,
            .cluster_high = (idx >> 16) & 0xFFFF,
            .cluster_low = idx & 0xFFFF,
            .filesize = 0};

    // copy nama
    for (uint8_t j = 0; j < sizeof(newEntry.name); j++)
    {
      newEntry.name[j] = request.name[j];
    }

    // cari cluster yang kosong
    int dirindex = 0;
    while (fat32_driver_state.dir_table_buf.table[dirindex].user_attribute == UATTR_NOT_EMPTY)
    {
      dirindex++;
    }

    fat32_driver_state.dir_table_buf.table[dirindex] = newEntry;

    struct FAT32DirectoryTable temp =
        {
            .table =
                {
                    {
                        // input new_entry to table
                        .attribute = ATTR_SUBDIRECTORY,
                        .user_attribute = UATTR_NOT_EMPTY,
                        .filesize = request.buffer_size,
                        .cluster_high = request.parent_cluster_number >> 16,
                        .cluster_low = request.parent_cluster_number & 0xFFFF,
                    }}};
    for (int i = 0; i < 8; i++)
    {
      temp.table[0].name[i] = request.name[i];
    }
    fat32_driver_state.fat_table.cluster_map[idx] = FAT32_FAT_END_OF_FILE;
    write_clusters(&temp, idx, 1);
    write_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);
    write_clusters(&fat32_driver_state.fat_table, 1, 1);
    return 0;
  }
  else
  {

    uint8_t dirindex = 0;
    while (fat32_driver_state.dir_table_buf.table[dirindex].user_attribute == UATTR_NOT_EMPTY)
    {
      dirindex++;
    }

    // cari cluster yang kosong
    uint8_t tempindex = idx;
    uint8_t nextIndex = idx;
    for (uint8_t i = 0; i < ceil(request.buffer_size / CLUSTER_SIZE); i++)
    {
      // YANG BERHUBUNGAN DENGAN FAT TABLE
      //  cari yang kosong pertama
      while (fat32_driver_state.fat_table.cluster_map[tempindex] != 0)
      {
        tempindex++;
      }
      // Apabila sudah ditemukan,maka masukkan ke dalam directory tablenya
      if (i == 0)
      {
        // buat entry baru
        struct FAT32DirectoryEntry new_entry = {
            .user_attribute = UATTR_NOT_EMPTY,
            .filesize = request.buffer_size,
            .cluster_high = (tempindex >> 16) & 0xFFFF,
            .cluster_low = tempindex & 0xFFFF,
        };

        for (uint8_t j = 0; j < 8; j++)
        {
          new_entry.name[j] = request.name[j];
        }
        for (uint8_t j = 0; j < 3; j++)
        {
          new_entry.ext[j] = request.ext[j];
        }
        fat32_driver_state.dir_table_buf.table[dirindex] = new_entry;
        write_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);
      }
      nextIndex = tempindex + 1;
      // cari yang kosong kedua
      while (fat32_driver_state.fat_table.cluster_map[nextIndex] != 0)
      {
        nextIndex++;
      }
      // buat entry baru
      if (i == ceil(request.buffer_size / CLUSTER_SIZE - 1))
      {
        fat32_driver_state.fat_table.cluster_map[tempindex] = FAT32_FAT_END_OF_FILE;
      }
      else
      {
        fat32_driver_state.fat_table.cluster_map[tempindex] = nextIndex;
      }
      write_clusters(request.buf + i * CLUSTER_SIZE, tempindex, 1);
      write_clusters(&fat32_driver_state.fat_table, 1, 1);
    }
    return 0;
  }
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