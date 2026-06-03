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

section .bss ; variáveis globais não inicializadas (inicializadas com zero por padrão)
    ; o stack pointer (ESP) precisa apontar para um endereço válido, mesmo que o kernel ainda não tenha sido carregado
    ; isso é necessário para que o GRUB possa empilhar os argumentos do kernel e outras informações importantes
align 16 ; alinhamento de 16 bytes para o stack pointer (ESP) — requisito do protocolo Multiboot

stack_bottom: ; marca o início da pilha do kernel
    resb 16384 ; reserva 16KB para a pilha do kernel (tamanho arbitrário, pode ser ajustado conforme necessário)
stack_top: ; marca o topo da pilha do kernel (stack grows downwards)

section .text ; seção de código executável
global _start ; ponto de entrada do kernel, o GRUB vai pular para esse endereço após carregar o kernel na memória
extern kernel_main ; declara a função kernel_main, que será definida em kernel.c e será o ponto de entrada principal do kernel após a inicialização

_start: ; o GRUB já terá carregado o kernel na memória e passado o controle para este ponto de entrada
    mov esp, stack_top ; inicializa o stack pointer (ESP) para o topo da pilha do kernel
    call kernel_main ; chama a função kernel_main, que é onde a execução do kernel realmente começa 
    
    cli ; desabilita interrupções para evitar comportamentos indesejados após o kernel_main retornar (embora kernel_main não deva retornar...)
.hang: ; loop infinito para manter o kernel rodando
    hlt ; instrucao de halt para economizar energia enquanto o kernel está "parado"
    jmp .hang ; repete o loop

    