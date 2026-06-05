bits 32
global irq1_wrapper
extern irq1_handler

irq1_wrapper:
    pusha               ; salva todos os registradores
    call irq1_handler   ; chama o handler em C
    popa                ; restaura todos os registradores
    iret                ; retorna da interrupção

section .note.GNU-stack noalloc noexec nowrite progbits