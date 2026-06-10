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
    pusha                       ; salva EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    mov eax, esp                ; captura ESP atual (aponta para os regs salvos)
    push eax                    ; passa como argumento para o handler C
    call timer_handler          ; retorna ESP do próximo processo em EAX
    add esp, 4                  ; limpa o argumento
    mov esp, eax                ; carrega ESP do próximo processo
    call pic_eoi_irq0           ; EOI antes de retornar
    popa                        ; restaura registradores do próximo processo
    iret                        ; retorna para o próximo processo
    
section .note.GNU-stack noalloc noexec nowrite progbits