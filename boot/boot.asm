bits 32 ; NASM: gera instruções de 32-bit

section .multiboot ; garante que fica nos primeiros 8KB
align 4 ; Multiboot exige que o header esteja alinhado a 4 bytes
    MBALIGN  equ 1 << 0 ; bit 0 setado → diz ao GRUB para alinhar módulos carregados em páginas de 4KB
    MEMINFO  equ 1 << 1 ; bit 1 setado → pede ao GRUB o mapa de memória da máquina

    MB_FLAGS equ MBALIGN | MEMINFO ; flags do header do Multiboot

    MB_MAGIC    equ 0x1BADB002  ; número que o GRUB procura nos primeiros 8KB hardcoded no protocolo Multiboot
    MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS) ; checksum do header do Multiboot (deve ser tal que a soma dos 3 seja zero)

    ; define doubleword — escreve 4 bytes na memória. O GRUB vai encontrar esses 12 bytes, 
    ; verificar o magic, o checksum e confirmar que o kernel Multiboot é válido.
    dd MB_MAGIC 
    dd MB_FLAGS
    dd MB_CHECKSUM