# Compiler & linker
ASM           = nasm
LIN           = ld
CC            = gcc

# Directory
SOURCE_FOLDER = src
OUTPUT_FOLDER = bin
ISO_NAME      = OS2024

# Flags
WARNING_CFLAG = -Wall -Wextra -Werror
DEBUG_CFLAG   = -fshort-wchar -g
STRIP_CFLAG   = -nostdlib -fno-stack-protector -nostartfiles -nodefaultlibs -ffreestanding
CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) -m32 -c -I$(SOURCE_FOLDER)
AFLAGS        = -f elf32 -g -F dwarf
LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386

DISK_NAME      = sample-image


run: all
	@rm $(OUTPUT_FOLDER)/*.o $(OUTPUT_FOLDER)/kernel
	@qemu-system-i386 -s -S -drive file=bin/sample-image.bin,format=raw,if=ide,index=0,media=disk -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso
all: build
build: iso
clean:
	rm -rf $(OUTPUT_FOLDER)/*.o $(OUTPUT_FOLDER)/*.object $(OUTPUT_FOLDER)/*.iso $(OUTPUT_FOLDER)/kernel

kernel: 
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/fat32.c -o $(OUTPUT_FOLDER)/fat32.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/disk.c -o $(OUTPUT_FOLDER)/disk.o

	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/keyboard.c -o $(OUTPUT_FOLDER)/keyboard.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/paging.c -o $(OUTPUT_FOLDER)/paging.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/portio.c -o $(OUTPUT_FOLDER)/portio.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/stdlib//string.c -o $(OUTPUT_FOLDER)/string.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/framebuffer.c -o $(OUTPUT_FOLDER)/framebuffer.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/gdt.c -o $(OUTPUT_FOLDER)/gdt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/idt.c -o $(OUTPUT_FOLDER)/idt.o
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/intsetup.s -o $(OUTPUT_FOLDER)/intsetup.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/interrupt.c -o $(OUTPUT_FOLDER)/interrupt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/kernel.c -o $(OUTPUT_FOLDER)/kernel.o
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/kernel-entrypoint.s -o $(OUTPUT_FOLDER)/kernel-entrypoint.o
	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/kernel
	@echo Linking object files and generate elf32...
	@rm -f *.o

iso: kernel
	@mkdir -p $(OUTPUT_FOLDER)/iso/boot/grub
	@cp $(OUTPUT_FOLDER)/kernel     $(OUTPUT_FOLDER)/iso/boot/
	@cp other/grub1                 $(OUTPUT_FOLDER)/iso/boot/grub/
	@cp $(SOURCE_FOLDER)/menu.lst   $(OUTPUT_FOLDER)/iso/boot/grub/
	# TODO: Create ISO image
	@genisoimage -R                  \
	-b boot/grub/grub1           \
		-no-emul-boot                \
		-boot-load-size 4            \
		-A os                        \
	-input-charset utf8          \
	-quiet                       \
	-boot-info-table             \
	-o $(OUTPUT_FOLDER)/OS2024.iso \
		$(OUTPUT_FOLDER)/iso
	@rm -r $(OUTPUT_FOLDER)/iso

disk:
	@qemu-img create -f raw $(OUTPUT_FOLDER)/$(DISK_NAME).bin 4M

inserter:
	@$(CC) -Wno-builtin-declaration-mismatch -g -I$(SOURCE_FOLDER) \
	$(SOURCE_FOLDER)/stdlib/string.c \
	$(SOURCE_FOLDER)/fat32.c \
	$(SOURCE_FOLDER)/external-inserter.c \
  -o $(OUTPUT_FOLDER)/inserter

user-shell:
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/crt0.s -o crt0.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/user-shell.c -o user-shell.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/stdlib/string.c -o string.o
	@$(LIN) -T $(SOURCE_FOLDER)/user-linker.ld -melf_i386 --oformat=binary \
	crt0.o user-shell.o string.o -o $(OUTPUT_FOLDER)/shell
	@echo Linking object shell object files and generate flat binary...
	@$(LIN) -T $(SOURCE_FOLDER)/user-linker.ld -melf_i386 --oformat=elf32-i386 \
	crt0.o user-shell.o string.o -o $(OUTPUT_FOLDER)/shell_elf
	@echo Linking object shell object files and generate ELF32 for debugging...
	@wc -c $(OUTPUT_FOLDER)/shell
	@rm -f *.o

insert-shell: inserter user-shell
	@echo Inserting shell into root directory...
	@cd $(OUTPUT_FOLDER); ./inserter shell 2 $(DISK_NAME).bin


# # MacOS (Apple Silicon - Bisa)
# # Compiler & linker
# ASM           = nasm
# LIN           = /opt/homebrew/Cellar/llvm/17.0.6_1/bin/ld.lld
# CC            = clang
# ISO     = mkisofs

# # Directory
# SOURCE_FOLDER = src
# OUTPUT_FOLDER = bin
# ISO_NAME      = OS2024

# # Flags
# WARNING_CFLAG = -Wall -Wextra -Werror
# DEBUG_CFLAG   = -fshort-wchar -g
# STRIP_CFLAG   = -nostdlib -fno-stack-protector -nodefaultlibs -ffreestanding -mno-sse -mno-avx
# TARGET_CFLAG  = --target=i386-elf -m32 
# CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) $(TARGET_CFLAG) -c -I$(SOURCE_FOLDER)
# AFLAGS        = -f elf32 -g -F dwarf
# LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386


# run: all
#  @rm $(OUTPUT_FOLDER)/*.o $(OUTPUT_FOLDER)/kernel
#  @qemu-system-i386 -s -S -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso
# all: build
# build: iso
# clean:
#  rm -rf $(OUTPUT_FOLDER)/*.o $(OUTPUT_FOLDER)/*.object $(OUTPUT_FOLDER)/*.iso $(OUTPUT_FOLDER)/kernel

# kernel: 
#  @$(CC) $(CFLAGS) $(SOURCE_FOLDER)/keyboard.c -o $(OUTPUT_FOLDER)/keyboard.o
#  @$(CC) $(CFLAGS) $(SOURCE_FOLDER)/portio.c -o $(OUTPUT_FOLDER)/portio.o
#  @$(CC) $(CFLAGS) $(SOURCE_FOLDER)/stdlib//string.c -o $(OUTPUT_FOLDER)/string.o
#  @$(CC) $(CFLAGS) $(SOURCE_FOLDER)/framebuffer.c -o $(OUTPUT_FOLDER)/framebuffer.o
#  @$(CC) $(CFLAGS) $(SOURCE_FOLDER)/gdt.c -o $(OUTPUT_FOLDER)/gdt.o
#  @$(CC) $(CFLAGS) $(SOURCE_FOLDER)/idt.c -o $(OUTPUT_FOLDER)/idt.o
#  @$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/intsetup.s -o $(OUTPUT_FOLDER)/intsetup.o
#  @$(CC) $(CFLAGS) $(SOURCE_FOLDER)/interrupt.c -o $(OUTPUT_FOLDER)/interrupt.o
#  @$(CC) $(CFLAGS) $(SOURCE_FOLDER)/kernel.c -o $(OUTPUT_FOLDER)/kernel.o
#  @$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/kernel-entrypoint.s -o $(OUTPUT_FOLDER)/kernel-entrypoint.o
#  @$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/kernel
#  @echo Linking object files and generate elf32...
#  @rm -f *.o

# iso: kernel
#  @mkdir -p $(OUTPUT_FOLDER)/iso/boot/grub
#  @cp $(OUTPUT_FOLDER)/kernel     $(OUTPUT_FOLDER)/iso/boot/
#  @cp other/grub1                 $(OUTPUT_FOLDER)/iso/boot/grub/
#  @cp $(SOURCE_FOLDER)/menu.lst   $(OUTPUT_FOLDER)/iso/boot/grub/
#  @$(ISO) -R                         \
#    -b boot/grub/grub1         \
#    -no-emul-boot              \
#    -boot-load-size 4          \
#    -A os                      \
#    -input-charset utf8        \
#    -quiet                     \
#    -boot-info-table           \
#    -o bin/OS2024.iso          \
#    bin/iso
#  @rm -r $(OUTPUT_FOLDER)/iso/