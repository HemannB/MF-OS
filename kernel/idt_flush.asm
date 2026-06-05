bits 32
global idt_flush

idt_flush:
    mov eax, [esp + 4]  ; pega o endereço do idt_ptr
    lidt [eax]          ; carrega a IDT na CPU
    ret