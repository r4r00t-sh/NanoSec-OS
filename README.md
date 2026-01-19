# NanoSec OS

**A custom operating system built from scratch - NOT based on Linux!**

## What This Is

NanoSec OS is a **completely custom operating system** with:
- Custom bootloader (Assembly)
- Custom kernel (C)
- Custom shell with security commands
- Built-in firewall and security monitoring

This is NOT a Linux distribution - it's a fully independent OS!

## Quick Build

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential nasm gcc-multilib qemu-system-x86

# Build
cd build
chmod +x build.sh
./build.sh all

# Run in QEMU
./build.sh run
```

## Or build kernel directly

```bash
cd kernel
make clean
make
make run
```

## Architecture

```
┌─────────────────────────────────┐
│  Shell Commands                 │
├─────────────────────────────────┤
│  NanoSec Kernel (C)             │
│  • Memory Manager               │
│  • VGA Driver                   │
│  • Keyboard Driver              │
│  • Firewall                     │
│  • Security Monitor             │
├─────────────────────────────────┤
│  Bootloader (x86 Assembly)      │
│  • Real Mode → Protected Mode   │
│  • GDT Setup                    │
│  • Disk Loading                 │
└─────────────────────────────────┘
```

## Shell Commands

| Command | Description |
|---------|-------------|
| `help` | Show commands |
| `status` | Security status |
| `firewall` | Firewall info |
| `memory` | Memory usage |
| `version` | OS version |
| `halt` | Shutdown |

## Files

- `boot/boot.asm` - 16-bit bootloader
- `kernel/kernel.c` - Main kernel
- `kernel/drivers/` - Hardware drivers
- `kernel/security/` - Security modules
- `kernel/mm/` - Memory manager

## License

MIT
