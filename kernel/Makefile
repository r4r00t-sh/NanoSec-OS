#
# NanoSec OS Kernel Makefile
# ============================
# Standard floppy boot version (~36KB kernel)
#

CC = gcc
AS = nasm
LD = ld

CFLAGS = -m32 -ffreestanding -fno-stack-protector -fno-builtin -fno-pic \
         -nostdlib -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -O2 -I.
LDFLAGS = -m elf_i386 -T linker.ld --oformat binary

ROOT_DIR = ..
BUILD_DIR = $(ROOT_DIR)/build
OBJ_DIR = $(BUILD_DIR)/obj
BOOT_DIR = $(ROOT_DIR)/boot

# Core kernel sources (fits in ~36KB floppy boot)
C_SOURCES = entry.c \
            kernel.c \
            shell.c \
            shell_pipe.c \
            sysinfo.c \
            env.c \
            history.c \
            net_commands.c \
            drivers/vga.c \
            drivers/keyboard.c \
            drivers/timer.c \
            drivers/speaker.c \
            drivers/rtc.c \
            drivers/serial.c \
            mm/memory.c \
            fs/ramfs.c \
            fs/editor.c \
            fs/fileutils.c \
            fs/permissions.c \
            fs/fhs.c \
            fs/commands.c \
            fs/man.c \
            fs/utils.c \
            fs/textproc.c \
            nash/nash.c \
            auth/users.c \
            security/firewall.c \
            security/monitor.c \
            security/advanced.c \
            boot_menu.c \
            drivers/video.c \
            graphics/login.c \
            graphics/desktop.c \
            net/ne2000.c \
            net/arp.c \
            net/ip.c \
            net/icmp.c \
            net/udp.c \
            net/dns.c

C_OBJECTS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SOURCES))

BOOTLOADER = $(BUILD_DIR)/boot.bin
KERNEL = $(BUILD_DIR)/kernel.bin
IMAGE = $(BUILD_DIR)/nanosec.img

.PHONY: all clean run directories

all: directories $(BOOTLOADER) $(KERNEL) $(IMAGE)
	@echo ""
	@echo "╔═══════════════════════════════════════════════════╗"
	@echo "║           NanoSec OS - Build Complete             ║"
	@echo "╠═══════════════════════════════════════════════════╣"
	@echo "║  Image: $(IMAGE)"
	@echo "║  Run:   make run                                  ║"
	@echo "╚═══════════════════════════════════════════════════╝"

directories:
	@mkdir -p $(OBJ_DIR)/drivers
	@mkdir -p $(OBJ_DIR)/mm
	@mkdir -p $(OBJ_DIR)/fs
	@mkdir -p $(OBJ_DIR)/auth
	@mkdir -p $(OBJ_DIR)/security
	@mkdir -p $(OBJ_DIR)/net
	@mkdir -p $(OBJ_DIR)/nash
	@mkdir -p $(OBJ_DIR)/graphics

$(BOOTLOADER): $(BOOT_DIR)/boot.asm
	@echo "[ASM] boot.asm"
	@$(AS) -f bin $< -o $@

$(OBJ_DIR)/%.o: %.c kernel.h
	@echo "[CC]  $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(C_OBJECTS)
	@echo "[LD]  kernel.bin"
	@$(LD) $(LDFLAGS) $^ -o $@
	@SIZE=$$(stat -c%s $@ 2>/dev/null || stat -f%z $@); \
	echo "Kernel size: $$SIZE bytes"

$(IMAGE): $(BOOTLOADER) $(KERNEL)
	@echo "[IMG] nanosec.img"
	@dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	@dd if=$(BOOTLOADER) of=$@ conv=notrunc 2>/dev/null
	@dd if=$(KERNEL) of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null

run: $(IMAGE)
	@qemu-system-i386 -fda $(IMAGE) -m 32M

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete."

