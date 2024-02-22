# The xv6 kernel starts executing in this file. This file is linked with
# the kernel C code, so it can refer to kernel symbols such as main().
# The boot block (bootasm.S and bootmain.c) jumps to entry below.
        
# Multiboot header, for multiboot boot loaders like GNU Grub.
# http://www.gnu.org/software/grub/manual/multiboot/multiboot.html
#
# Using GRUB 2, you can boot xv6 from a file stored in a
# Linux file system by copying kernel or kernelmemfs to /boot
# and then adding this menu entry:
#
# menuentry "xv6" {
# 	insmod ext2
# 	set root='(hd0,msdos1)'
# 	set kernel='/boot/kernel'
# 	echo "Loading ${kernel}..."
# 	multiboot ${kernel} ${kernel}
# 	boot
# }

#include "asm.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"

# Multiboot header.  Data to direct multiboot loader.
.p2align 2
.text
.globl multiboot_header
multiboot_header:
  #define magic 0x1badb002
  #define flags 0
  .long magic
  .long flags
  .long (-magic-flags)

# By convention, the _start symbol specifies the ELF entry point.
# Since we haven't set up virtual memory yet, our entry point is
# the physical address of 'entry'.
.globl _start
_start = V2P_WO(entry) 
# The _start symbol is set to the physical address of the entry function, indicating the entry point for the kernel. This is where the kernel starts executing after being loaded by the boot loader.

# Entering xv6 on boot processor, with paging off.

# Now segmentation is set up and kernel is loaded in main memory 
.globl entry
entry:
# 1 level paging -> 1024 entries ka pgdir points where each entry points to 4 MB page
  # Turn on page size extension for 4Mbyte pages
  movl    %cr4, %eax
  orl     $(CR4_PSE), %eax
  movl    %eax, %cr4 
  # SET PSE to 1 means 1 level paging 
  # Set page directory
  movl    $(V2P_WO(entrypgdir)), %eax # I had virtual addr of entrypgdir cause its part of main.c and we need physical addr to store in cr3 register
  movl    %eax, %cr3 #  It sets the page directory (cr3) to point to the physical address of the page directory (entrypgdir).
  # Turn on paging.
  movl    %cr0, %eax # Paging is enabled by setting the paging bit in the control register (cr0).
  orl     $(CR0_PG|CR0_WP), %eax # enable PG bit in CR0 to enable paging
  movl    %eax, %cr0

  # Set up the stack pointer.
  movl $(stack + KSTACKSIZE), %esp #  It sets up the stack pointer (esp) to point to the top of the kernel stack.

  # Jump to main(), and switch to executing at
  # high addresses. The indirect call is needed because
  # the assembler produces a PC-relative instruction
  # for a direct jump.
  mov $main, %eax #  Finally, it jumps to the main function, effectively starting the execution of the kernel.
  jmp *%eax

.comm stack, KSTACKSIZE
