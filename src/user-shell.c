#include <stdint.h>
#include "header/cpu/fat32.h"
#include "header/stdlib/string.h"

// SYSCALLS
#define READ 0
#define READ_DIR 1
#define WRITE 2
#define DELETE 3
#define KEYBOARD_BUFFER 4
#define PUTS_CHAR 5
#define PUTS 6
#define KEYBOARD_ACTIVATE 7
#define KEYBOARD_UP_ROW 8
#define KEYBOARD_RESET 9
#define CLEAR_SCREEN 10

#define STACK_SIZE 100
struct DirectoryState
{
    char name[8];
    uint32_t cluster_number;
    uint32_t parent_cluster_number;
};

static struct DirectoryState current_directory = {
    .name = "root",
    .cluster_number = ROOT_CLUSTER_NUMBER,
    .parent_cluster_number = ROOT_CLUSTER_NUMBER,
};

static struct DirectoryState directory_stack[STACK_SIZE];
static int top = -1;

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

void push_directory(struct DirectoryState directory)
{
    if (top >= STACK_SIZE - 1)
    {
        syscall(PUTS, (uint32_t) "Stack Overflow", 14, 0xC);
        return;
    }
    directory_stack[++top] = directory;
}

struct DirectoryState pop_directory()
{
    if (top < 0)
    {
        syscall(PUTS, (uint32_t) "Stack Underflow", 15, 0xC);
        struct DirectoryState empty_directory = {.cluster_number = 0};
        return empty_directory;
    }
    return directory_stack[top--];
}

void find(char *name, uint32_t cluster_number, char *curent_dir, uint32_t next_cluster)
{
    struct FAT32DirectoryTable buf;
    struct FAT32DriverRequest request = {.buf = &buf,
                                         .parent_cluster_number = cluster_number,
                                         .buffer_size = sizeof(struct FAT32DirectoryTable)};
    memcpy(request.name, curent_dir, sizeof(request.name));

    syscall(1, (uint32_t)&request, 0, 0);
    for (int i = 2; i < 64; i++)
    {
        if (buf.table[i].user_attribute == UATTR_NOT_EMPTY)
        {
            if (strcmp(buf.table[i].name, name))
            {
                syscall(6, (uint32_t)buf.table[i].name, 8, 0xF);
                return;
            }
            else if (buf.table[i].attribute == ATTR_SUBDIRECTORY)
            {
                find(name, next_cluster, buf.table[i].name, buf.table[i].cluster_low | (buf.table[i].cluster_high << 16));
            }
        }
    }
}

void cd(char *name)
{
    if (strcmp(name, ".."))
    {
        if (top < 0)
            return;
        current_directory = pop_directory();
        return;
    }
    struct FAT32DirectoryTable buf;
    struct FAT32DriverRequest request = {.buf = &buf,
                                         .parent_cluster_number = current_directory.parent_cluster_number,
                                         .buffer_size = sizeof(struct FAT32DirectoryTable)};
    memcpy(request.name, current_directory.name, sizeof(current_directory.name));

    syscall(READ_DIR, (uint32_t)&request, 0, 0);
    for (int i = 2; i < 64; i++)
    {
        if (buf.table[i].user_attribute == UATTR_NOT_EMPTY)
        {
            if (buf.table[i].attribute == ATTR_SUBDIRECTORY)
            {
                if (strcmp(buf.table[i].name, name))
                {
                    push_directory(current_directory);
                    current_directory.parent_cluster_number = current_directory.cluster_number;
                    current_directory.cluster_number = buf.table[i].cluster_low | (buf.table[i].cluster_high << 16);
                    memcpy(current_directory.name, name, sizeof(current_directory.name));
                    return;
                }
            }
        }
    }
    syscall(PUTS, (uint32_t) "Directory not found", 19, 0xC);
}

void cat(char *name)
{
    struct ClusterBuffer buf;
    struct FAT32DriverRequest request = {
        .buf = &buf,
        .parent_cluster_number = current_directory.cluster_number,
        .buffer_size = 0x100000,
    };

    memcpy(request.name, name, sizeof(request.name));

    syscall(READ, (uint32_t)&request, 0, 0);

    int i = 0;
    while (buf.buf[i] != '\0')
    {
        if (buf.buf[i] == '\n')
        {
            syscall(KEYBOARD_UP_ROW, 0, 0, 0);
        }
        else
            syscall(PUTS_CHAR, (uint32_t)buf.buf[i], 0xF, 0);
        i++;
    }
}

void rm(char *name)
{
    struct FAT32DriverRequest request = {
        .ext = "\0\0\0",
        .parent_cluster_number = current_directory.cluster_number,
        .buffer_size = 0,
    };

    memcpy(request.name, name, sizeof(request.name));

    syscall(DELETE, (uint32_t)&request, 0, 0);
}

void mkdir(char *name)
{
    struct FAT32DriverRequest request = {
        .ext = "\0\0\0",
        .parent_cluster_number = current_directory.cluster_number,
        .buffer_size = 0,
    };

    memcpy(request.name, name, sizeof(request.name));

    syscall(WRITE, (uint32_t)&request, 0, 0);
}

void ls()
{
    struct FAT32DirectoryTable buf;
    struct FAT32DriverRequest request = {.buf = &buf,
                                         .parent_cluster_number = current_directory.parent_cluster_number,
                                         .buffer_size = sizeof(struct FAT32DirectoryTable)};
    memcpy(request.name, current_directory.name, sizeof(request.name));

    syscall(READ_DIR, (uint32_t)&request, 0, 0);
    for (int i = 2; i < 64; i++)
    {
        if (buf.table[i].user_attribute == UATTR_NOT_EMPTY)
        {
            if (buf.table[i].attribute == ATTR_SUBDIRECTORY)
                syscall(PUTS_CHAR, (uint32_t)'/', 0xF, 0);
            syscall(PUTS, (uint32_t)buf.table[i].name, 8, 0xF);
            syscall(PUTS_CHAR, (uint32_t)' ', 0xF, 0);
        }
    }
}
void show_home()
{
    syscall(PUTS, (uint32_t) "root@kajijOSta", 14, 0x2);
    syscall(PUTS_CHAR, (uint32_t)':', 0x8, 0);
    syscall(PUTS_CHAR, (uint32_t)'~', 0x1, 0);
    syscall(PUTS_CHAR, (uint32_t)'$', 0x8, 0);
    syscall(PUTS_CHAR, (uint32_t)' ', 0x8, 0);
}

void handle_command(char *input)
{
    char temp[256];
    memcpy(temp, input, 256);
    get_string(input, 0);
    if (strcmp(temp, "clear"))
    {
        syscall(CLEAR_SCREEN, 0, 0, 0);
        syscall(KEYBOARD_RESET, 0, 0, 0);
    }
    else if (strcmp(temp, "ls"))
    {
        syscall(KEYBOARD_UP_ROW, 0, 0, 0);
        ls();
        syscall(KEYBOARD_UP_ROW, 0, 0, 0);
    }
    else if (strcmp(input, "mkdir"))
    {
        char *test = get_string(temp, 1);
        mkdir(test);
        syscall(KEYBOARD_UP_ROW, 0, 0, 0);
    }
    else if (strcmp(input, "rm"))
    {
        char *test = get_string(temp, 1);
        rm(test);
        syscall(KEYBOARD_UP_ROW, 0, 0, 0);
    }
    else if (strcmp(input, "cat"))
    {
        syscall(KEYBOARD_UP_ROW, 0, 0, 0);
        char *test = get_string(temp, 1);
        cat(test);
        syscall(KEYBOARD_UP_ROW, 0, 0, 0);
    }
    else if (strcmp(input, "cd"))
    {
        syscall(KEYBOARD_UP_ROW, 0, 0, 0);
        char *test = get_string(temp, 1);
        cd(test);
    }
    else if (strcmp(input, "find"))
    {
        syscall(KEYBOARD_UP_ROW, 0, 0, 0);
        char *test = get_string(temp, 1);
        find(test, ROOT_CLUSTER_NUMBER, current_directory.name, ROOT_CLUSTER_NUMBER);
        syscall(KEYBOARD_UP_ROW, 0, 0, 0);
    }
    else
    {
        syscall(PUTS, (uint32_t) "command not found", 17, 0xC);
        syscall(KEYBOARD_UP_ROW, 0, 0, 0);
    }
    show_home();
}

int main(void)
{
    memcpy(current_directory.name, "root", 4);
    show_home();
    syscall(KEYBOARD_ACTIVATE, 0, 0, 0);
    char input[256];
    int i = 0;
    while (true)
    {
        char buf;
        bool print_mode;
        syscall(KEYBOARD_BUFFER, (uint32_t)&buf, (uint32_t)&print_mode, 0);
        if (buf && print_mode)
        {
            if (buf == '\n')
            {
                handle_command(input);
                i = 0;
                for (int j = 0; j < 256; j++)
                    input[j] = '\0';
            }
            else
            {
                syscall(PUTS, (uint32_t)&buf, 1, 0xF);
                input[i++] = buf;
            }
        }
        else if (buf && !print_mode)
        {
            if (i > 0)
            {
                i--;
                input[i] = '\0';
            }
        }
    }
    return 0;
}