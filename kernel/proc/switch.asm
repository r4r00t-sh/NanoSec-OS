; NanoSec OS - Context Switch
; =============================
; Low-level context switching between processes

[BITS 32]

; void switch_context(uint32_t *old_esp, uint32_t new_esp)
; Saves current context, switches stack, restores new context
global switch_context
switch_context:
    ; Get parameters
    mov eax, [esp + 4]      ; old_esp pointer
    mov edx, [esp + 8]      ; new_esp value
    
    ; Save callee-saved registers on current stack
    push ebp
    push ebx
    push esi
    push edi
    
    ; Save current ESP to old_esp
    mov [eax], esp
    
    ; Switch to new stack
    mov esp, edx
    
    ; Restore callee-saved registers from new stack
    pop edi
    pop esi
    pop ebx
    pop ebp
    
    ; Return (to new process's return address)
    ret
