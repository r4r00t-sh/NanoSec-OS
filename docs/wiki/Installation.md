# Installation

## Prerequisites

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential nasm qemu-system-x86 grub-pc-bin xorriso mtools
```

### Fedora
```bash
sudo dnf install make gcc nasm qemu xorriso mtools grub2-tools
```

### Arch Linux
```bash
sudo pacman -S base-devel nasm qemu grub xorriso mtools
```

### Windows (WSL2)
1. Install WSL2 with Ubuntu
2. Run the Ubuntu prerequisites above

## Building

### Quick Build (Recommended)
```bash
git clone https://github.com/r4r00t-sh/NanoSec-OS.git
cd NanoSec-OS
make clean && make full && make run-iso
```

### Manual Build

#### Floppy Image
```bash
cd kernel
make clean
make
make run
```

#### GRUB ISO (for larger kernel)
```bash
cd kernel
make -f Makefile.multiboot clean
make -f Makefile.multiboot
make -f Makefile.multiboot iso
make -f Makefile.multiboot run-iso
```

## First Boot

1. Wait for GRUB menu (or floppy boot)
2. NanoSec OS will load
3. Login with:
   - **Username:** `root`
   - **Password:** `root`
4. Type `help` to see available commands

## Troubleshooting

### Build Errors
- Ensure all prerequisites are installed
- Use WSL2 on Windows, not native Windows

### QEMU Issues
- Try `-enable-kvm` for faster emulation (Linux only)
- Check QEMU is properly installed

### Boot Failures
- Use the GRUB ISO method for large kernels
- Check serial output for debug messages
