# Architecture

## Overview

NanoSec OS is a 32-bit x86 operating system with the following architecture:

```
┌─────────────────────────────────────────────────────┐
│                   User Space                        │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐   │
│  │  Shell  │ │  Nash   │ │ Editors │ │ Network │   │
│  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘   │
└───────┼───────────┼───────────┼───────────┼────────┘
        │     System Call Interface (INT 0x80)        │
┌───────┼───────────┼───────────┼───────────┼────────┐
│       ▼           ▼           ▼           ▼        │
│  ┌─────────────────────────────────────────────┐   │
│  │              Kernel Core                    │   │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐       │   │
│  │  │ Process │ │ Memory  │ │   VFS   │       │   │
│  │  │ Manager │ │ Manager │ │         │       │   │
│  │  └─────────┘ └─────────┘ └─────────┘       │   │
│  └─────────────────────────────────────────────┘   │
│                                                     │
│  ┌─────────────────────────────────────────────┐   │
│  │              Device Drivers                 │   │
│  │  VGA │ Keyboard │ IDE │ NE2000 │ Timer     │   │
│  └─────────────────────────────────────────────┘   │
│                                                     │
│  ┌─────────────────────────────────────────────┐   │
│  │              Hardware Abstraction           │   │
│  │  IDT │ GDT │ PIC │ I/O Ports              │   │
│  └─────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
                        │
                   ┌────▼────┐
                   │ Hardware│
                   └─────────┘
```

---

## Directory Structure

```
kernel/
├── kernel.c          # Main kernel entry
├── kernel.h          # Global declarations
├── shell.c           # Command shell
├── shell_pipe.c      # Pipe/redirect handling
│
├── cpu/              # CPU-related
│   ├── idt.c         # Interrupt Descriptor Table
│   └── isr.asm       # Interrupt Service Routines
│
├── drivers/          # Hardware drivers
│   ├── vga.c         # VGA text/graphics
│   ├── keyboard.c    # PS/2 keyboard
│   ├── timer.c       # PIT timer
│   ├── ide.c         # IDE/ATA disk
│   └── ne2000.c      # Network card
│
├── fs/               # Filesystems
│   ├── ramfs.c       # RAM filesystem
│   ├── utils.c       # File utilities
│   └── textproc.c    # Text processing
│
├── mm/               # Memory management
│   ├── memory.c      # Physical memory
│   └── paging.c      # Virtual memory
│
├── net/              # Network stack
│   ├── arp.c         # ARP protocol
│   ├── ip.c          # IP protocol
│   ├── icmp.c        # ICMP (ping)
│   ├── tcp.c         # TCP protocol
│   └── udp.c         # UDP protocol
│
├── proc/             # Process management
│   ├── process.c     # Process handling
│   ├── signal.c      # Signal handling
│   └── syscall.c     # System calls
│
├── security/         # Security features
│   ├── firewall.c    # Packet filtering
│   └── monitor.c     # Security monitoring
│
└── nash/             # Nash scripting
    └── nash.c        # Nash interpreter
```

---

## Boot Process

1. **BIOS** loads bootloader (or GRUB)
2. **Bootloader** switches to protected mode
3. **Kernel** initializes:
   - GDT (Global Descriptor Table)
   - IDT (Interrupt Descriptor Table)
   - Memory manager
   - Device drivers
   - Filesystem
4. **Shell** starts with login prompt

---

## Memory Layout

```
0x00000000 ┌─────────────────┐
           │   Reserved      │ (1MB)
0x00100000 ├─────────────────┤
           │   Kernel Code   │
           ├─────────────────┤
           │   Kernel Data   │
           ├─────────────────┤
           │   Kernel Heap   │
           ├─────────────────┤
           │   Page Tables   │
           ├─────────────────┤
           │   User Space    │
           ├─────────────────┤
           │   Stack         │
0xFFFFFFFF └─────────────────┘
```

---

## Interrupt Handling

| IRQ | Handler | Description |
|-----|---------|-------------|
| 0 | Timer | System tick (100Hz) |
| 1 | Keyboard | Key press/release |
| 12 | Mouse | PS/2 mouse |
| 14 | IDE | Disk operations |
| 0x80 | Syscall | System calls |

---

## System Calls

```c
// Example system call usage
int result = syscall(SYS_WRITE, fd, buffer, size);
```

| Number | Name | Description |
|--------|------|-------------|
| 0 | exit | Exit process |
| 1 | read | Read from file |
| 2 | write | Write to file |
| 3 | open | Open file |
| 4 | close | Close file |
| 5 | fork | Create process |
