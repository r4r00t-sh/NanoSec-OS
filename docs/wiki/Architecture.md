# Architecture

NanoSec OS is a 32-bit x86 operating system built from scratch. This page explains how the system works at a technical level.

---

## System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                      USER SPACE                             │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │  Shell   │ │   Nash   │ │  nedit   │ │  nping   │       │
│  │ (shell.c)│ │ Scripting│ │  Editor  │ │ Network  │       │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘       │
└───────┼────────────┼────────────┼────────────┼─────────────┘
        │            │            │            │
        └────────────┴─────┬──────┴────────────┘
                           │
              ┌────────────▼────────────┐
              │  System Call Interface  │
              │       (INT 0x80)        │
              └────────────┬────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│                      KERNEL SPACE                           │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                  KERNEL CORE                        │   │
│  │  ┌───────────┐ ┌───────────┐ ┌───────────┐         │   │
│  │  │  Process  │ │  Memory   │ │    VFS    │         │   │
│  │  │  Manager  │ │  Manager  │ │ (ramfs)   │         │   │
│  │  └───────────┘ └───────────┘ └───────────┘         │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                 NETWORK STACK                       │   │
│  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐           │   │
│  │  │ TCP │ │ UDP │ │ICMP │ │ IP  │ │ ARP │           │   │
│  │  └─────┘ └─────┘ └─────┘ └─────┘ └─────┘           │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                  DEVICE DRIVERS                     │   │
│  │  VGA │ Keyboard │ Timer │ IDE │ NE2000 │ Mouse     │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              HARDWARE ABSTRACTION                   │   │
│  │  GDT │ IDT │ PIC │ I/O Ports │ Paging              │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                           │
              ┌────────────▼────────────┐
              │        HARDWARE         │
              │  CPU │ RAM │ Disk │ NIC │
              └─────────────────────────┘
```

---

## Boot Process

### Stage 1: BIOS/Bootloader
1. **BIOS** performs POST (Power-On Self Test)
2. BIOS loads **bootloader** from disk (first 512 bytes)
3. Bootloader (or GRUB) switches CPU to **Protected Mode** (32-bit)

### Stage 2: Kernel Initialization
```
kernel_main() in kernel.c
│
├── 1. vga_init()         → Initialize VGA text mode (80x25)
├── 2. gdt_init()         → Setup Global Descriptor Table
├── 3. idt_init()         → Setup Interrupt Descriptor Table
├── 4. timer_init()       → Start PIT at 100Hz
├── 5. keyboard_init()    → Enable PS/2 keyboard
├── 6. memory_init()      → Initialize memory manager
├── 7. fs_init()          → Create RAM filesystem with FHS
├── 8. user_init()        → Setup user authentication
├── 9. net_init()         → Initialize network stack
└── 10. shell_run()       → Start interactive shell
```

### Stage 3: Login & Shell
- User sees login prompt
- After authentication, shell starts
- Shell reads commands and executes them

---

## Memory Map

```
Physical Address        Description
─────────────────────────────────────────
0x00000000              ┌─────────────────┐
                        │   Real Mode     │
                        │   IVT & BDA     │
                        │   (Reserved)    │
0x00100000 (1MB)        ├─────────────────┤
                        │   Kernel Code   │  (.text section)
                        │   Functions     │
                        ├─────────────────┤
                        │   Kernel Data   │  (.data, .bss)
                        │   Global Vars   │
                        ├─────────────────┤
                        │   Kernel Heap   │  (Dynamic alloc)
                        │   kmalloc()     │
                        ├─────────────────┤
                        │   Page Tables   │
                        ├─────────────────┤
                        │   Process Space │  (Per-process)
                        │   - Code        │
                        │   - Data        │
                        │   - Stack       │
0xC0000000              ├─────────────────┤
                        │   Kernel Map    │  (Higher half)
0xFFFFFFFF              └─────────────────┘
```

---

## Interrupt System

### Interrupt Descriptor Table (IDT)

The IDT maps interrupt numbers to handler functions:

| Vector | Type | Handler | Description |
|--------|------|---------|-------------|
| 0-31 | Exception | `isr0-31` | CPU exceptions (divide error, page fault, etc.) |
| 32 | IRQ0 | `timer_handler` | System timer (100Hz tick) |
| 33 | IRQ1 | `keyboard_handler` | Keyboard input |
| 44 | IRQ12 | `mouse_handler` | PS/2 Mouse |
| 46 | IRQ14 | `ide_handler` | IDE disk operations |
| 128 | Syscall | `syscall_handler` | System calls (INT 0x80) |

### Interrupt Flow
```
1. Hardware generates interrupt (e.g., key press)
2. CPU looks up handler in IDT
3. CPU saves state and jumps to handler
4. Handler processes interrupt
5. Handler sends EOI (End of Interrupt) to PIC
6. CPU restores state and continues
```

---


---

## Graphics Architecture

NanoSec OS implements a unified graphics subsystem that abstracts hardware differences between VESA and VGA modes.

### Unified Graphics API (`gfx.c`)
The kernel uses a single API for all drawing operations, automatically dispatching to the active driver:

```c
// Auto-detects VESA or VGA
int gfx_init_auto(uint32_t mb_magic, uint32_t *mb_info);

// Drawing primitives
void gfx_pixel(int x, int y, uint32_t color);
void gfx_draw_rect(int x, int y, int w, int h, uint32_t color);
void gfx_draw_text(int x, int y, const char *str, uint32_t color);
```

### Drivers
1.  **VESA Driver (`vesa.c`)**
    *   **Resolution:** 800x600
    *   **Color Depth:** 32-bit True Color (0xRRGGBB)
    *   **Mechanism:** Multiboot Framebuffer (LFB)
    *   **Features:** Hardware-accelerated memory mapping, 8x8 font

2.  **VGA Driver (`video.c`)**
    *   **Resolution:** 320x200
    *   **Color Depth:** 256 colors (Palette)
    *   **Mechanism:** VGA Mode 13h (0xA0000)
    *   **Features:** Legacy compatibility

---

## Filesystem Architecture

### RAM Filesystem (ramfs)

```c
// File node structure
typedef struct {
    char name[32];          // Filename
    uint8_t type;           // NODE_FILE or NODE_DIR
    int parent;             // Parent directory index
    uint32_t size;          // File size in bytes
    uint8_t data[4096];     // File contents
    uint32_t created;       // Creation timestamp
    uint32_t modified;      // Modification timestamp
} fs_node_t;
```

### FHS Directory Structure
```
/
├── bin/       # User commands (ls, cat, grep...)
├── sbin/      # System commands (reboot, shutdown...)
├── etc/       # Configuration files
│   ├── passwd
│   ├── hostname
│   └── motd
├── home/      # User home directories
│   └── guest/
├── var/       # Variable data
│   └── log/
├── tmp/       # Temporary files
├── dev/       # Device files
├── proc/      # Process information
└── root/      # Root user home
```

---

## Network Stack

### Protocol Layers

```
┌─────────────────────────────────────┐
│         Application Layer           │
│    (ping, dns client, http)         │
├─────────────────────────────────────┤
│         Transport Layer             │
│       TCP (tcp.c) │ UDP (udp.c)     │
├─────────────────────────────────────┤
│          Network Layer              │
│    IP (ip.c) │ ICMP (icmp.c)        │
├─────────────────────────────────────┤
│          Link Layer                 │
│   ARP (arp.c) │ Ethernet Frames     │
├─────────────────────────────────────┤
│          Driver Layer               │
│        NE2000 (ne2000.c)            │
└─────────────────────────────────────┘
```

### Packet Flow (Sending)
```
1. Application creates data
2. TCP/UDP adds transport header
3. IP adds network header
4. ARP resolves MAC address
5. Driver sends Ethernet frame
```

---

## Process Management

### Process Structure
```c
typedef struct process {
    uint32_t pid;           // Process ID
    char name[32];          // Process name
    uint32_t esp;           // Stack pointer
    uint32_t ebp;           // Base pointer
    uint32_t eip;           // Instruction pointer
    page_directory_t *pd;   // Page directory
    uint8_t state;          // RUNNING, READY, BLOCKED
    struct process *next;   // Next in scheduler queue
} process_t;
```

### Scheduler
- **Round-robin** scheduling
- Timer interrupt triggers context switch
- Each process gets time slice (~10ms)

---

## Security Features

### Stack Protection
```c
// Stack canary check
#define STACK_CHK_GUARD 0xDEADBEEF

void __stack_chk_fail(void) {
    panic("Stack smashing detected!");
}
```

### Address Space Layout Randomization (ASLR)
- Randomizes heap and stack base addresses
- Makes buffer overflow exploits harder

### Firewall
- Packet filtering by port and protocol
- Rules stored in kernel memory
- Checked on each incoming packet

---

## Source Code Map

| Directory | Files | Purpose |
|-----------|-------|---------|
| `kernel/` | `kernel.c`, `shell.c` | Core kernel and shell |
| `kernel/cpu/` | `idt.c`, `isr.asm` | Interrupt handling |
| `kernel/drivers/` | `vesa.c`, `video.c`, `keyboard.c` | Hardware drivers |
| `kernel/graphics/` | `gfx.c`, `login.c`, `desktop.c` | Graphics subsystem |
| `kernel/fs/` | `ramfs.c`, `utils.c` | Filesystem |
| `kernel/mm/` | `memory.c`, `paging.c` | Memory management |
| `kernel/net/` | `tcp.c`, `ip.c`, `arp.c` | Network stack |
| `kernel/proc/` | `process.c`, `syscall.c` | Process management |
| `kernel/security/` | `firewall.c`, `monitor.c` | Security features |
| `kernel/nash/` | `nash.c` | Nash scripting language |

---

## Key Data Structures

### Command Structure (Shell)
```c
typedef struct {
    const char *name;       // Command name
    const char *desc;       // Description
    void (*handler)(const char *args);  // Function pointer
} command_t;
```

### Network Packet
```c
typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
    uint8_t data[1500];
} ethernet_frame_t;
```

---

## Building the Kernel

### Compilation Steps
```
1. Assemble boot.asm → boot.bin
2. Compile *.c → *.o (object files)
3. Link all *.o → kernel.bin
4. Combine boot.bin + kernel.bin → nanosec.img
```

### Memory Layout After Linking
```
Sections:
.text    0x100000    # Code (kernel functions)
.rodata  (follows)   # Read-only data (strings)
.data    (follows)   # Initialized data
.bss     (follows)   # Uninitialized data (zeroed)
```
