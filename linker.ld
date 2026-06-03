/* Linker script para o MF-OS */
ENTRY(_start)

/*abre o bloco que define como organizar a memória*/
SECTIONS {
    . = 0x100000; /* o . é o "location counter" -> diz ao linker onde começar a colocar o código. 0x100000 = 1MB */
    .text BLOCK(4K) : ALIGN(4K) { /* define a seção .text com alinhamento de 4KB */
        *(.multiboot) /* puxa a seção .multiboot de todos os arquivos objeto vem primeiro para garantir que o header Multiboot fica nos primeiros 8KB */
        *(.text) /* puxa todo o código executável */
    }
    .rodata BLOCK(4K) : ALIGN(4K) { /* define a seção .rodata com alinhamento de 4KB */
        *(.rodata) /* puxa todos os dados somente de leitura */
    }

    .data BLOCK(4K) : ALIGN(4K) { /* define a seção .data com alinhamento de 4KB */
        *(.data) /* puxa todos os dados de leitura e escrita */
    }

    .bss BLOCK(4K) : ALIGN(4K) { /* define a seção .bss com alinhamento de 4KB */
        *(COMMON)   /* variáveis globais não inicializadas declaradas sem static */
        *(.bss)     /* dados não inicializados em geral — incluindo nossa stack */
    }
}