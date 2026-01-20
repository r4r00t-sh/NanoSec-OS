#
# NanoSec OS - Root Makefile
# ============================
# Multiboot build (GRUB) is the default
#

.PHONY: all clean run help

# Default: Build and run multiboot kernel
all:
	@$(MAKE) -C kernel -f Makefile.multiboot

# Build, create ISO, and run in QEMU - all in one!
run: all
	@$(MAKE) -C kernel -f Makefile.multiboot run-iso

# Clean all build artifacts
clean:
	@$(MAKE) -C kernel -f Makefile.multiboot clean
	@echo "All clean."

# Help
help:
	@echo ""
	@echo "NanoSec OS Build System"
	@echo "======================="
	@echo ""
	@echo "  make       - Build kernel"
	@echo "  make run   - Build and run in QEMU"
	@echo "  make clean - Remove all build artifacts"
	@echo ""
