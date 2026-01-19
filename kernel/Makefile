#
# NanoSec OS Kernel Makefile - Complete with Networking
#

CC = gcc
AS = nasm
LD = ld

CFLAGS = -m32 -ffreestanding -fno-stack-protector -fno-builtin -fno-pic \
         -nostdlib -Wall -Wno-unused-function -O2 -I. -I./net
LDFLAGS = -m elf_i386 -T linker.ld --oformat binary

ROOT_DIR = ..
BUILD_DIR = $(ROOT_DIR)/build/work/kernel
OBJ_DIR = $(BUILD_DIR)/obj
BOOT_DIR = $(ROOT_DIR)/boot

# Entry must be first!
C_SOURCES = entry.c \
            kernel.c \
            shell.c \
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
            auth/users.c \
            security/firewall.c \
            security/monitor.c \
            security/advanced.c \
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

all: directories $(BOOTLOADER) $(KERNEL) $(IMAGE)
	@echo ""
	@echo "╔═══════════════════════════════════════════════════╗"
	@echo "║           NanoSec OS Build Complete!              ║"
	@echo "╠═══════════════════════════════════════════════════╣"
	@echo "║  Login: root / root                               ║"
	@echo "║  60+ commands including real networking!          ║"
	@echo "║  Run: make run-net (for networking)               ║"
	@echo "╚═══════════════════════════════════════════════════╝"

directories:
	@mkdir -p $(OBJ_DIR)/drivers
	@mkdir -p $(OBJ_DIR)/mm
	@mkdir -p $(OBJ_DIR)/fs
	@mkdir -p $(OBJ_DIR)/auth
	@mkdir -p $(OBJ_DIR)/security
	@mkdir -p $(OBJ_DIR)/net

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
	@echo "Kernel: $$(stat -c%s $@) bytes"

$(IMAGE): $(BOOTLOADER) $(KERNEL)
	@echo "[IMG] nanosec.img"
	@dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	@dd if=$(BOOTLOADER) of=$@ conv=notrunc 2>/dev/null
	@dd if=$(KERNEL) of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null

run: $(IMAGE)
	qemu-system-i386 -fda $(IMAGE) -m 32M

run-net: $(IMAGE)
	qemu-system-i386 -fda $(IMAGE) -m 32M \
		-netdev user,id=net0,hostfwd=tcp::5555-:80 \
		-device ne2k_isa,netdev=net0

run-tap: $(IMAGE)
	qemu-system-i386 -fda $(IMAGE) -m 32M \
		-netdev tap,id=net0,ifname=tap0,script=no,downscript=no \
		-device ne2k_isa,netdev=net0

run-serial: $(IMAGE)
	qemu-system-i386 -fda $(IMAGE) -m 32M -serial stdio

run-debug: $(IMAGE)
	qemu-system-i386 -fda $(IMAGE) -m 32M -serial stdio -d int

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all directories run run-net run-tap run-serial run-debug clean
