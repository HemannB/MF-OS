bits 32

global irq1_wrapper
extern irq1_handler

irq1_wrapper:
    pusha                   ; salva todos os registradores gerais na stack
    call irq1_handler       ; chama o handler do teclado em C
    popa                    ; restaura todos os registradores
    iret                    ; retorno especial de interrupção — restaura EIP, CS e EFLAGS

global irq0_wrapper
extern timer_handler
extern pic_eoi_irq0

irq0_wrapper:
    pusha                   ; salva todos os registradores gerais na stack
    call timer_handler      ; incrementa o contador de ticks
    call pic_eoi_irq0       ; avisa o PIC que o handler terminou — sem isso IRQ0 para de disparar
    popa                    ; restaura todos os registradores
    iret                    ; retorno especial de interrupção

section .note.GNU-stack noalloc noexec nowrite progbits