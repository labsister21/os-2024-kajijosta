# 𝗞𝗔𝗝𝗜𝗝𝗢𝗦𝗧𝗔

> **Tugas Besar IF2230 - Sistem Operasi**

> This program is a fully functional operating system developed from scratch, featuring interrupt handling, file system implementation, paging, user mode, shell creation, and multitasking capabilities.

![OS Image](./images/os.jpg)

## Table of Contents

1. [Overview](#overview)
2. [Technologies Used](#technologies-used)
3. [Setting Up](#setting-up)
4. [Structure](#structure)
5. [Status](#status)
6. [Ch. 0 - Toolchain, Kernel, GDT](#ch-0---toolchain-kernel-gdt)
7. [Ch. 1 - Interrupt, Driver, File System](#ch-1---interrupt-driver-file-system)
8. [Ch. 2 - Paging, User Mode, Shell](#ch-2---paging-user-mode-shell)
9. [Ch. 3 - Process, Scheduler, Multitasking](#ch-3---process-scheduler-multitasking)
10. [Authors](#created-by)
11. [Acknowledgements](#acknowledgements)

## Overview

This project is the implementation of a simple operating system from scratch. It provides a guided journey through fundamental OS development concepts and components across multiple chapters, covering topics such as toolchain setup, kernel creation, interrupt handling, driver development, file system implementation, memory management with paging, and multitasking support.

Key objectives of this project include:

- Understanding the boot process and how to load an OS using GRUB.
- Developing a simple kernel in C, managing memory, and handling basic hardware interrupts.
- Creating drivers for essential hardware components such as the keyboard and text framebuffer.
- Implementing a FAT32 file system to manage files and directories.
- Enabling paging to support virtual memory.
- Introducing user mode to run applications and creating a basic shell for user interaction.
- Managing processes and implementing a scheduler for multitasking.

## Important Notice

> [!IMPORTANT]\
> Windows users will experience issues if not using `Windows Subsystem for Linux`. For the best experience, it is recommended to use WSL or a native Linux installation. Apple Silicon users should follow the alternate toolchain and workflow provided specifically for that architecture.

> [!IMPORTANT]\
> There are three versions of this project released :+1:. Make sure you are using the latest version, which has been tested thoroughly. Regularly check the project repository for updates to ensure you have the latest fixes and improvements.

## Technologies Used

- C

## Setting Up

<details>
<summary>:eyes: Prerequisites</summary>
Before you start, ensure you have the following prerequisites installed on your system:

1. **Operating System:**

   - Linux (preferred), macOS, or Windows with WSL (Windows Subsystem for Linux)

2. **Development Tools:**

   - Netwide Assembler (NASM)
   - GNU C Compiler (GCC)
   - GNU Linker (LD)
   - QEMU
   - System i386
   - GNU Make
   - genisoimage
   - GDB (GNU Debugger)

3. **Additional Software:**

   - Git (for version control)
   - Visual Studio Code or any other code editor

4. **Knowledge Requirements:**
   - Basic understanding of C programming
   - Familiarity with assembly language
   - Understanding of computer architecture and operating system concepts

</details>

<details>
<summary>:eyes: Installation</summary>
Install The Required Dependencies
#### Clone the Repository:

```sh
git clone https://github.com/yourusername/os-project.git
cd os-project
```

#### Install GCC, Binutils, Make:

```sh
sudo apt-get update
sudo apt-get install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo
```

#### Install QEMU:

```sh
sudo apt-get install qemu
```

</details>

<details>
<summary>:eyes: Usage</summary>
Navigate to the source directory:

```sh
cd src
```

Build the OS:

```sh
make run
```

</details>

## Structure

```bash
OS-2024-Kernel-Project
┣ .github
┃ ┗ .keep
┣ .vscode
┃ ┣ launch.json
┃ ┣ settings.json
┃ ┗ tasks.json
┣ bin
┃ ┣ .gitignore
┃ ┣ inserter
┃ ┣ os2024.iso
┃ ┣ sample-image.bin
┃ ┣ sample-image.bin.7z
┃ ┣ shell
┃ ┣ shell_elf
┃ ┗ storage.bin
┣ image
┃ ┗ image.jpg
┣ other
┣ src
┃ ┣ header
┃ ┃ ┣ cpu.h
┃ ┃ ┣ disk.h
┃ ┃ ┣ fat32.h
┃ ┃ ┣ gdt.h
┃ ┃ ┣ idt.h
┃ ┃ ┣ interrupt.h
┃ ┃ ┣ keyboard.h
┃ ┃ ┣ paging.h
┃ ┃ ┣ portio.h
┃ ┃ ┗ string.h
┃ ┣ stdlib
┃ ┃ ┣ crt0.s
┃ ┃ ┣ disk.c
┃ ┃ ┣ external-insertsort.s
┃ ┃ ┣ fat32.c
┃ ┃ ┣ framebuffer.c
┃ ┃ ┣ gdt.c
┃ ┃ ┣ idt.c
┃ ┃ ┣ interrupt.c
┃ ┃ ┣ initsetup.s
┃ ┃ ┣ kernel-entrypoint.c
┃ ┃ ┣ kernel.c
┃ ┃ ┣ keyboard.c
┃ ┃ ┣ linker.ld
┃ ┃ ┣ menu.lst
┃ ┃ ┣ paging.c
┃ ┃ ┣ portio.c
┃ ┃ ┣ string.c
┃ ┃ ┣ user-linker.ld
┃ ┃ ┗ user-shell.c
┃ ┗ .gitignore
┗ README.md
```

## Status

<table>
  <tr style="background-color: #f2f2f2;">
    <th>Chapter</th>
    <th>Sub-chapter</th>
    <th>Status</th>
  </tr>
  <tr style="background-color: #ffffff;">
    <td rowspan="3">Ch. 0 - Toolchain, Kernel, GDT</td>
    <td>Repository & Toolchain</td>
    <td>complete</td>
  </tr>
  <tr style="background-color: #e7f3ff;">
    <td>Kernel</td>
    <td>complete</td>
  </tr>
  <tr style="background-color: #ffffff;">
    <td>Global Descriptor Table</td>
    <td>complete</td>
  </tr>
  <tr style="background-color: #e7f3ff;">
    <td rowspan="5">Ch. 1 - Interrupt, Driver, File System</td>
    <td>Short Note: Kernel Development</td>
    <td>complete</td>
  </tr>
  <tr style="background-color: #ffffff;">
    <td>Driver Text Framebuffer</td>
    <td>complete</td>
  </tr>
  <tr style="background-color: #e7f3ff;">
    <td>Interrupt</td>
    <td>complete</td>
  </tr>
  <tr style="background-color: #ffffff;">
    <td>Keyboard Driver</td>
    <td>complete</td>
  </tr>
  <tr style="background-color: #e7f3ff;">
    <td>File System: FAT32 - IF2230 Edition</td>
    <td>complete</td>
  </tr>
  <tr style="background-color: #ffffff;">
    <td rowspan="3">Ch. 2 - Paging, User Mode, Shell</td>
    <td>Paging</td>
    <td>complete</td>
  </tr>
  <tr style="background-color: #e7f3ff;">
    <td>User Mode</td>
    <td>complete</td>
  </tr>
  <tr style="background-color: #ffffff;">
    <td>Shell</td>
    <td>complete</td>
  </tr>
  <tr style="background-color: #e7f3ff;">
    <td rowspan="3">Ch. 3 - Process, Scheduler, Multitasking</td>
    <td>Process</td>
    <td>incomplete</td>
  </tr>
  <tr style="background-color: #ffffff;">
    <td>Scheduler</td>
    <td>incomplete</td>
  </tr>
  <tr style="background-color: #e7f3ff;">
    <td>Multitasking</td>
    <td>incomplete</td>
  </tr>
</table>

> [!WARNING]\
> Chapter 3 might not function as expected due to its `incomplete` status. Future development may address these issues and expand the content further.

## 📃 Chapter 0: Toolchain, Kernel, GDT

Chapter 0 serves as an initial introduction to kernel development. Most of this chapter is spent setting up the Repository & Toolchain, creating a basic Kernel that can be compiled, and concluding with the Global Descriptor Table.

The guide primarily uses Linux and x64 as the development environment. The instructions have been tested on x64 architecture, Windows 10 & 11 with WSL2 running Ubuntu 20.04/22.04, Ubuntu Desktop 20.04, and Arch Linux.

For Apple Silicon, an alternate toolchain and workflow are provided for development on Apple Silicon. These instructions have been tested on MacOS Ventura 13.2.1.

By the end of Chapter 0, the operating system can be run using QEMU.

## 📃 Chapter 1: Interrupt, Driver, File System

Chapter 1 focuses on creating the necessary Interrupt system to develop the Keyboard Driver and File System. Chapter 1 begins by creating a Text Framebuffer.

By the end of Chapter 1, the operating system is able to receive and display keyboard input. Additionally, the file system is also be implemented in Chapter 1. All features implemented in Chapter 1 will be used to create the CLI Shell in Chapter 2.

## 📃 Chapter 2: Paging, User Mode, Shell

Chapter 2 aims to create a User Program that provides a Command Line Interface. Before creating the user program, the kernel first implements Paging and User Mode.

Once both parts are completed, the Shell component implements System Calls and a Command Line Interface to provide an interface to the user. At the end of Chapter 2, the operating system have a shell that users can utilize to execute commands and operate the computer.

## 📃 Chapter 3: Process, Scheduler, Multitasking

Chapter 3, as the Grand Finale, implements the Multitasking feature. To achieve this, Interrupts and Memory Manager from the previous chapters are being utilized again, along with additional steps such as preparing Processes and a Scheduler.

The process begins with the creation of Processes, providing isolation of resources between programs. The Scheduler being created performs periodic context switches using a specific algorithm. Once both features are completed, the operating system automatically performs Multitasking. By the end of Chapter 3, the operating system is able to run multiple programs concurrently.

## Created by

| Name                           | NIM      | Connect                                                |
| ------------------------------ | -------- | ------------------------------------------------------ |
| Jonathan Emmanuel Saragih      | 13522121 | [@JonathanSaragih](https://github.com/JonathanSaragih) |
| Satriadhikara Panji Yudhistira | 13522125 | [@satriadhikara](https://github.com/satriadhikara)     |
| Andhika Fadillah               | 13522128 | [@Andhikafdh](https://github.com/Andhikafdh)           |
| Attara Majesta Ayub            | 13522139 | [@attaramajesta](https://github.com/attaramajesta)     |

## Acknowledgements

- Tuhan Yang Maha Esa
- Dosen Pengampu Mata Kuliah Sistem Operasi Institut Teknologi Bandung 2024
- Asisten Mata Kuliah Sistem Operasi Institut Teknologi Bandung 2024
