bits 32
global context_switch

; void context_switch(uint32_t *old_esp, uint32_t *new_esp)
context_switch:
    ; salva registradores callee-saved na stack atual
    push ebp
    push ebx
    push esi
    push edi

    ; salva ESP atual em *old_esp
    mov eax, [esp + 20]     ; old_esp (4 regs × 4 bytes + 4 do ret = 20)
    mov [eax], esp

    ; carrega ESP do novo processo
    mov ecx, [esp + 24]     ; new_esp
    mov esp, [ecx]

    ; restaura registradores do novo processo
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret                     ; retorna para onde o novo processo estava

section .note.GNU-stack noalloc noexec nowrite progbits