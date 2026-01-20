# Building from Source

Complete guide to building NanoSec OS from source code.

---

## Prerequisites

### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential nasm qemu-system-x86 grub-pc-bin xorriso mtools git
```

### Linux (Fedora)
```bash
sudo dnf install make gcc nasm qemu xorriso mtools grub2-tools git
```

### Linux (Arch)
```bash
sudo pacman -S base-devel nasm qemu grub xorriso mtools git
```

### Windows (WSL2)
1. Install WSL2: `wsl --install`
2. Install Ubuntu from Microsoft Store
3. Run Ubuntu prerequisites above

### macOS
```bash
brew install nasm qemu xorriso mtools
# Note: Cross-compiler may be needed
```

---

## Getting the Source

```bash
git clone https://github.com/r4r00t-sh/NanoSec-OS.git
cd NanoSec-OS
```

---

## Build Options

### Option 1: Quick Build (Recommended)
```bash
make clean && make full && make run-iso
```

This will:
1. Clean previous builds
2. Build the multiboot kernel
3. Create GRUB ISO
4. Launch in QEMU

### Option 2: Floppy Image
```bash
cd kernel
make clean
make
make run
```

Best for small kernels (<64KB after bootloader).

### Option 3: GRUB ISO (Manual)
```bash
cd kernel
make -f Makefile.multiboot clean
make -f Makefile.multiboot
make -f Makefile.multiboot iso
make -f Makefile.multiboot run-iso
```

---

## Build Targets

| Target | Command | Description |
|--------|---------|-------------|
| `make` | Build kernel | Compile kernel binary |
| `make clean` | Clean build | Remove compiled files |
| `make run` | Run floppy | Boot floppy in QEMU |
| `make iso` | Create ISO | Build bootable ISO |
| `make run-iso` | Run ISO | Boot ISO in QEMU |
| `make full` | Full build | Complete multiboot build |

---

## Build Output

After successful build:
```
build/
├── kernel.bin          # Kernel binary (floppy)
├── kernel.elf          # Kernel ELF (multiboot)
├── nanosec.img         # Floppy image
├── nanosec.iso         # Bootable ISO
└── obj/                # Object files
```

---

## Makefile Configuration

### Key Variables

```makefile
# Compiler settings
CC = gcc
CFLAGS = -m32 -ffreestanding -nostdlib -Wall -Wextra

# Assembler
ASM = nasm
ASMFLAGS = -f elf32

# Linker
LD = ld
LDFLAGS = -m elf_i386 -T linker.ld
```

### Adding New Source Files

1. Create your `.c` file in appropriate directory
2. Add to `C_SOURCES` in `Makefile`:
```makefile
C_SOURCES = kernel.c \
            shell.c \
            your_new_file.c \
```
3. Add declarations to `kernel.h`
4. Rebuild: `make clean && make`

---

## Cross-Compiling

If your system is 64-bit and has issues:

### Build i686 Cross-Compiler
```bash
# Download binutils and gcc source
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Build binutils
cd binutils-2.xx
./configure --target=$TARGET --prefix=$PREFIX --disable-nls
make && make install

# Build gcc
cd gcc-xx.x
./configure --target=$TARGET --prefix=$PREFIX --disable-nls \
    --enable-languages=c --without-headers
make all-gcc && make install-gcc
```

Then use:
```makefile
CC = i686-elf-gcc
LD = i686-elf-ld
```

---

## Debugging

### Serial Output
```bash
# Run with serial console
qemu-system-i386 -cdrom nanosec.iso -serial stdio
```

### GDB Debugging
```bash
# Terminal 1: Start QEMU with GDB server
qemu-system-i386 -cdrom nanosec.iso -s -S

# Terminal 2: Connect GDB
gdb kernel.elf
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

### Debug Symbols
Ensure `-g` flag in CFLAGS for debug info:
```makefile
CFLAGS += -g
```

---

## Troubleshooting

### "Triple fault" or immediate reboot
- Check GDT/IDT setup
- Verify stack is properly aligned
- Check interrupt handlers

### "Kernel too large"
- Use GRUB multiboot instead of floppy
- Optimize code size with `-Os`

### QEMU hangs
- Add `-d int` to see interrupts
- Check for infinite loops

### Missing symbols during link
- Verify function declarations in `kernel.h`
- Check all source files are in `C_SOURCES`

---

## CI/CD

The project uses GitHub Actions for automated builds:

```yaml
# .github/workflows/ci.yml
- Build on every push
- Test kernel compilation
- Run static analysis
```

View build status on GitHub Actions tab.
