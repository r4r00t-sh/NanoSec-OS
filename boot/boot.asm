; NanoSec OS - Bootloader (Simple Multi-Track)
; Loads ~54KB kernel across multiple floppy tracks

[BITS 16]
[ORG 0x7C00]

start:
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [BOOT_DRIVE], dl
    
    mov si, MSG_BOOT
    call print
    
    ; Reset disk
    xor ax, ax
    int 0x13
    
    mov si, MSG_LOAD
    call print
    
    ; Load kernel to segment 0x1000
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    
    ; === Track 0, Head 0, Sectors 2-18 (17 sectors) ===
    mov ah, 0x02
    mov al, 17
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error
    add bx, 17*512
    
    ; === Track 0, Head 1, Sectors 1-18 (18 sectors) ===
    mov ah, 0x02
    mov al, 18
    mov ch, 0
    mov cl, 1
    mov dh, 1
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error
    add bx, 18*512
    
    ; === Track 1, Head 0, Sectors 1-18 (18 sectors) ===
    mov ah, 0x02
    mov al, 18
    mov ch, 1
    mov cl, 1
    mov dh, 0
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error
    add bx, 18*512
    
    ; === Track 1, Head 1, Sectors 1-18 (18 sectors) ===
    mov ah, 0x02
    mov al, 18
    mov ch, 1
    mov cl, 1
    mov dh, 1
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error
    add bx, 18*512
    
    ; === Track 2, Head 0, Sectors 1-18 (18 sectors) ===
    mov ah, 0x02
    mov al, 18
    mov ch, 2
    mov cl, 1
    mov dh, 0
    mov dl, [BOOT_DRIVE]
    int 0x13
    ; Ignore error - may be past kernel end
    
    ; Total: 17+18+18+18+18 = 89 sectors = ~45KB
    
    mov si, MSG_OK
    call print
    
    ; Enable A20
    in al, 0x92
    or al, 2
    out 0x92, al
    
    ; Protected mode
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm_start

print:
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

disk_error:
    mov si, MSG_ERR
    call print
.hang:
    hlt
    jmp .hang

; GDT
gdt_start:
    dq 0
gdt_code:
    dw 0xFFFF, 0x0000
    db 0x00, 10011010b, 11001111b, 0x00
gdt_data:
    dw 0xFFFF, 0x0000
    db 0x00, 10010010b, 11001111b, 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[BITS 32]
pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    jmp 0x10000

BOOT_DRIVE: db 0
MSG_BOOT: db 'NanoSec', 13, 10, 0
MSG_LOAD: db 'Loading', 0
MSG_OK:   db '.OK', 13, 10, 0
MSG_ERR:  db ' ERR', 0

times 510-($-$$) db 0
dw 0xAA55
