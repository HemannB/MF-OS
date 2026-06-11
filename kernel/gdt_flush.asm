bits 32
global gdt_flush

gdt_flush:
    mov eax, [esp + 4] ; Carrega o argumento do ponteiro gdt_ptr
    lgdt [eax]         ; Carrega o GDTR na CPU
    mov ax, 0x10       ; 0x10 = seletor do data segment (índice 2 na GDT) ; 0x10 em binário é 0000 0000 0001 0000 os bits 3-15 são o índice (2), bits 0-1 são o RPL (ring 0), bit 2 é TI (0 = GDT).
    mov ds, ax         ; Atualiza os registradores de segmento para usar o novo GDT
    mov es, ax   
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush   ; 0x08 = seletor do code segment (índice 1 na GDT) ; O salto é necessário para atualizar o CS com o novo seletor do code segment. O endereço .flush é um rótulo que aponta para a próxima instrução, garantindo que o código continue executando após a atualização do CS.
.flush:
    ret

section .note.GNU-stack noalloc noexec nowrite progbits