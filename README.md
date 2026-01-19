# NanoSec OS

<p align="center">
  <img src="https://img.shields.io/badge/OS-x86-blue" alt="x86">
  <img src="https://img.shields.io/badge/Language-C%20%7C%20Assembly-green" alt="C/ASM">
  <img src="https://img.shields.io/badge/Bootloader-GRUB%20Multiboot-orange" alt="GRUB">
  <img src="https://img.shields.io/badge/License-MIT-yellow" alt="MIT">
</p>

A security-focused hobby operating system written from scratch in C and x86 Assembly.

## ✨ Features

### Core OS
- **Preemptive Multitasking** - Round-robin scheduler with context switching
- **Virtual Memory** - Paging with memory protection
- **System Calls** - INT 0x80 interface
- **VGA Text & Graphics** - Mode 13h (320x200x256)
- **PS/2 Keyboard & Mouse** - Interrupt-driven

### Filesystem
- **RAM Filesystem** - In-memory file storage
- **FAT32 Support** - Read external drives
- **IDE/ATA Driver** - Hard disk access

### Networking
- **NE2000 Driver** - Network card support
- **Full TCP/IP Stack** - ARP, IP, ICMP, UDP, TCP
- **DNS Client** - Hostname resolution

### Security
- **User Authentication** - Login system with passwords
- **Firewall** - Packet filtering
- **ASLR** - Address space layout randomization
- **Stack Protection** - Canary-based overflow detection
- **Audit Logging** - Command tracking

## 🚀 Building

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt install build-essential nasm qemu-system-x86 grub-pc-bin xorriso mtools
```

### Build & Run (Floppy Image)
```bash
cd kernel
make clean
make
make run
```

### Build & Run (GRUB ISO - recommended for large kernel)
```bash
cd kernel
make -f Makefile.multiboot clean
make -f Makefile.multiboot
make -f Makefile.multiboot iso
make -f Makefile.multiboot run-iso
```

## 📁 Project Structure

```
nanosec-os/
├── boot/               # Bootloader
│   ├── boot.asm        # Floppy bootloader
│   ├── multiboot.asm   # GRUB multiboot header
│   └── grub/           # GRUB config
├── kernel/             # Kernel source
│   ├── cpu/            # IDT, interrupts
│   ├── drivers/        # VGA, keyboard, IDE, etc.
│   ├── fs/             # Filesystems
│   ├── mm/             # Memory management
│   ├── net/            # Network stack
│   ├── proc/           # Process management
│   ├── security/       # Security features
│   └── gui/            # Window manager
└── README.md
```

## 🔑 Default Login
- **Username:** `root`
- **Password:** `root`

## 📝 Commands

| Command | Description |
|---------|-------------|
| `help` | List commands |
| `ls` | List files |
| `cat` | View file |
| `edit` | Text editor |
| `ps` | Process list |
| `nping` | Ping utility |
| `nifconfig` | Network config |
| `firewall` | Manage firewall |

## 📜 License

MIT License - See [LICENSE](LICENSE)

## 🤝 Contributing

Contributions welcome! Please open an issue or PR.
