# NanoSec OS

<p align="center">
  <img src="https://img.shields.io/badge/OS-x86-blue" alt="x86">
  <img src="https://img.shields.io/badge/Language-C%20%7C%20Assembly-green" alt="C/ASM">
  <img src="https://img.shields.io/badge/Bootloader-GRUB%20Multiboot-orange" alt="GRUB">
  <img src="https://img.shields.io/badge/License-MIT-yellow" alt="MIT">
  <a href="https://github.com/r4r00t-sh/NanoSec-OS/wiki"><img src="https://img.shields.io/badge/Docs-Wiki-blueviolet" alt="Wiki"></a>
</p>

A security-focused hobby operating system written from scratch in C and x86 Assembly, featuring a Unix-like environment with custom Nash scripting language.

## 📚 Documentation

**[📖 Read the Wiki](https://github.com/r4r00t-sh/NanoSec-OS/wiki)** - Comprehensive documentation including:
- [Installation Guide](https://github.com/r4r00t-sh/NanoSec-OS/wiki/Installation)
- [Commands Reference](https://github.com/r4r00t-sh/NanoSec-OS/wiki/Commands)
- [Nash Scripting Language](https://github.com/r4r00t-sh/NanoSec-OS/wiki/Nash-Scripting-Language)
- [Architecture](https://github.com/r4r00t-sh/NanoSec-OS/wiki/Architecture)

## ✨ Features

### Core OS
- **Preemptive Multitasking** - Round-robin scheduler with context switching
- **Virtual Memory** - Paging with memory protection
- **System Calls** - INT 0x80 interface
- **VGA Text & Graphics** - Mode 13h (320x200x256)
- **PS/2 Keyboard & Mouse** - Interrupt-driven

### Unix-like Shell
- **Pipes & Redirects** - `cmd1 | cmd2`, `cmd > file`, `cmd >> file`
- **Command Chaining** - `cmd1 && cmd2`, `cmd1 || cmd2`, `cmd1 ; cmd2`
- **FHS Directory Structure** - `/bin`, `/etc`, `/home`, `/var`, etc.
- **50+ Commands** - ls, cd, mkdir, rm, cp, mv, find, grep, and more

### Nash Scripting Language
Custom scripting language with unique syntax:
```nash
-- Variables
@name = "World"
print "Hello, @name"

-- Conditionals
when @name eq World do
    print "Match!"
end

-- Loops
repeat 5 times
    print "Loop!"
end
```

### Filesystem
- **RAM Filesystem** - In-memory hierarchical file storage
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

### Quick Build (GRUB ISO - Recommended)
```bash
make clean && make full && make run-iso
```

### Alternative: Floppy Image
```bash
cd kernel && make clean && make && make run
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
│   ├── fs/             # Filesystems (ramfs, utils, textproc)
│   ├── mm/             # Memory management
│   ├── net/            # Network stack (TCP/IP)
│   ├── proc/           # Process management
│   ├── security/       # Security features
│   ├── nash/           # Nash scripting language
│   └── gui/            # Window manager
├── docs/               # Documentation & sample scripts
└── .github/workflows/  # CI/CD pipeline
```

## 🔑 Default Login
- **Username:** `root`
- **Password:** `root`

## 📝 Commands

### File Operations
| Command | Description |
|---------|-------------|
| `ls` | List files |
| `cd` | Change directory |
| `mkdir` | Create directory |
| `rm` / `rm -rf` | Remove file/directory |
| `cp` / `mv` | Copy / Move |
| `cat` | View file |
| `touch` | Create file |
| `find` | Search files |
| `stat` | File info |

### Text Processing
| Command | Description |
|---------|-------------|
| `grep` | Search text |
| `head` / `tail` | First/last lines |
| `wc` | Word count |
| `sort` / `uniq` | Sort / Remove duplicates |
| `cut` | Extract columns |
| `tr` | Translate characters |
| `sed` | Stream editor |
| `diff` | Compare files |

### System
| Command | Description |
|---------|-------------|
| `df` / `du` | Disk usage |
| `ps` | Process list |
| `env` | Environment vars |
| `history` | Command history |
| `man` | Manual pages |

### Networking
| Command | Description |
|---------|-------------|
| `nping` | Ping utility |
| `nifconfig` | Network config |
| `firewall` | Manage firewall |

### Scripting
| Command | Description |
|---------|-------------|
| `nash script.nsh` | Run Nash script |

## 🔧 Nash Scripting Language

Nash is a custom scripting language with unique syntax:

| Feature | Syntax |
|---------|--------|
| Variables | `@name = "value"` |
| Print | `print "Hello @name"` |
| Conditionals | `when @a eq @b do ... otherwise ... end` |
| Loops | `repeat 5 times ... end` |
| Run command | `run ls` |
| Comments | `-- comment` or `:: comment` |
| Operators | `eq`, `ne`, `gt`, `lt` |

## 📜 License

MIT License - See [LICENSE](LICENSE)

## 🤝 Contributing

Contributions welcome! Please open an issue or PR.
