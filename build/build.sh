#!/bin/bash
#
# NanoSec OS Build Script
# ========================
# Builds a CUSTOM kernel OS from scratch (NOT Linux!)
#

set -e

VERSION="1.0.0"

# Directories
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build/work"
OUTPUT_DIR="$PROJECT_DIR/output"
KERNEL_DIR="$PROJECT_DIR/kernel"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m'

log() { echo -e "${GREEN}[BUILD]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

# Check dependencies
check_deps() {
    echo -e "${CYAN}"
    echo "╔═══════════════════════════════════════════╗"
    echo "║     NanoSec OS Build System v${VERSION}        ║"
    echo "║     Custom Kernel (Not Linux!)            ║"
    echo "╚═══════════════════════════════════════════╝"
    echo -e "${NC}"
    
    log "Checking dependencies..."
    
    local deps="gcc nasm ld dd"
    for dep in $deps; do
        if command -v $dep &> /dev/null; then
            echo "  ✓ $dep"
        else
            error "Missing: $dep (install build-essential and nasm)"
        fi
    done
    
    # Check for 32-bit support
    if ! gcc -m32 -E - < /dev/null &> /dev/null; then
        error "32-bit GCC support missing. Install: sudo apt install gcc-multilib"
    fi
    echo "  ✓ gcc-multilib (32-bit support)"
}

# Build custom kernel
build_kernel() {
    log "Building NanoSec custom kernel..."
    
    cd "$KERNEL_DIR"
    make clean 2>/dev/null || true
    make
    
    if [ -f "$BUILD_DIR/kernel/nanosec.img" ]; then
        log "Kernel built successfully!"
    else
        error "Kernel build failed!"
    fi
}

# Create ISO
create_iso() {
    log "Creating bootable ISO..."
    
    mkdir -p "$OUTPUT_DIR"
    mkdir -p "$BUILD_DIR/iso/boot/grub"
    
    # Copy disk image
    cp "$BUILD_DIR/kernel/nanosec.img" "$BUILD_DIR/iso/boot/"
    
    # Create GRUB config for CD boot
    cat > "$BUILD_DIR/iso/boot/grub/grub.cfg" << 'EOF'
set timeout=3
set default=0

menuentry "NanoSec OS" {
    multiboot /boot/nanosec.img
}

menuentry "NanoSec OS (Debug)" {
    multiboot /boot/nanosec.img
    set debug=all
}
EOF

    # Create ISO if grub-mkrescue is available
    if command -v grub-mkrescue &> /dev/null; then
        grub-mkrescue -o "$OUTPUT_DIR/nanosec-os-${VERSION}.iso" "$BUILD_DIR/iso" 2>/dev/null
        log "ISO created: $OUTPUT_DIR/nanosec-os-${VERSION}.iso"
    else
        log "grub-mkrescue not found, skipping ISO (use floppy image instead)"
    fi
    
    # Copy floppy image to output
    cp "$BUILD_DIR/kernel/nanosec.img" "$OUTPUT_DIR/"
    log "Floppy image: $OUTPUT_DIR/nanosec.img"
}

# Test in QEMU
run_qemu() {
    log "Starting QEMU..."
    
    if [ -f "$OUTPUT_DIR/nanosec.img" ]; then
        qemu-system-i386 -fda "$OUTPUT_DIR/nanosec.img" -m 16M
    elif [ -f "$BUILD_DIR/kernel/nanosec.img" ]; then
        qemu-system-i386 -fda "$BUILD_DIR/kernel/nanosec.img" -m 16M
    else
        error "No bootable image found! Run: ./build.sh all"
    fi
}

# Clean
clean() {
    log "Cleaning..."
    rm -rf "$BUILD_DIR"
    rm -rf "$OUTPUT_DIR"
    cd "$KERNEL_DIR" && make clean 2>/dev/null || true
    log "Done!"
}

# Usage
usage() {
    echo "NanoSec OS Build Script"
    echo ""
    echo "Usage: $0 <command>"
    echo ""
    echo "Commands:"
    echo "  all     - Full build (kernel + image)"
    echo "  kernel  - Build kernel only"
    echo "  iso     - Create ISO image"
    echo "  run     - Run in QEMU"
    echo "  clean   - Clean build files"
    echo ""
}

# Main
case "${1:-}" in
    all)
        check_deps
        build_kernel
        create_iso
        echo ""
        echo -e "${GREEN}╔════════════════════════════════════════════╗${NC}"
        echo -e "${GREEN}║       Build Complete!                      ║${NC}"
        echo -e "${GREEN}╠════════════════════════════════════════════╣${NC}"
        echo -e "${GREEN}║  Run: ./build.sh run                       ║${NC}"
        echo -e "${GREEN}║  Or:  qemu-system-i386 -fda output/*.img   ║${NC}"
        echo -e "${GREEN}╚════════════════════════════════════════════╝${NC}"
        ;;
    kernel)
        check_deps
        build_kernel
        ;;
    iso)
        create_iso
        ;;
    run)
        run_qemu
        ;;
    clean)
        clean
        ;;
    *)
        usage
        ;;
esac
