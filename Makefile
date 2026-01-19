#
# NanoSec OS - Root Makefile
# ============================
# Centralized build system
#

.PHONY: all clean run run-debug iso run-iso help

# Default target
all:
	@$(MAKE) -C kernel

# Create bootable ISO (uses GRUB, for larger kernel)
iso:
	@$(MAKE) -C kernel -f Makefile.multiboot iso

# Run in QEMU (floppy boot)
run:
	@$(MAKE) -C kernel run

# Run with serial debug output
run-debug:
	@$(MAKE) -C kernel run-debug

# Run from ISO (GRUB boot)
run-iso:
	@$(MAKE) -C kernel -f Makefile.multiboot run-iso

# Clean all build artifacts
clean:
	@$(MAKE) -C kernel clean
	@echo "All clean."

# Build full kernel with all features (GRUB multiboot)
full:
	@$(MAKE) -C kernel -f Makefile.multiboot

# Help
help:
	@echo ""
	@echo "NanoSec OS Build System"
	@echo "======================="
	@echo ""
	@echo "  make          - Build kernel (floppy boot, ~36KB)"
	@echo "  make run      - Build and run in QEMU"
	@echo "  make run-debug- Run with serial console output"
	@echo ""
	@echo "  make full     - Build full kernel (GRUB, ~100KB+)"
	@echo "  make iso      - Create bootable ISO"
	@echo "  make run-iso  - Run ISO in QEMU"
	@echo ""
	@echo "  make clean    - Remove all build artifacts"
	@echo ""
