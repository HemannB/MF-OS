#include <stdint.h>
#include <stddef.h>
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "isr.h"
#include "timer.h"
#include "heap.h"
#include "paging.h"
#include "process.h"

// Funções para ler e escrever em portas de E/S, usadas para interagir com o hardware do sistema, como o controlador de vídeo e o teclado
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Função para ler um byte de uma porta de E/S, usada para obter informações do hardware, como o estado do teclado
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

// Definições para o driver de terminal VGA
#define VGA_ADDR  ((volatile uint16_t*) 0xB8000) // Buffer de vídeo para texto 
#define VGA_COLS  80 // Número de colunas 
#define VGA_ROWS  25 // Número de linhas
#define CMD_MAX 128 // Tamanho máximo do comando para o shell

// Cores do VGA
typedef enum {
    VGA_BLACK = 0, VGA_BLUE, VGA_GREEN, VGA_CYAN,
    VGA_RED, VGA_MAGENTA, VGA_BROWN, VGA_LIGHT_GREY,
    VGA_DARK_GREY, VGA_LIGHT_BLUE, VGA_LIGHT_GREEN,
    VGA_LIGHT_CYAN, VGA_LIGHT_RED, VGA_LIGHT_MAGENTA,
    VGA_LIGHT_BROWN, VGA_WHITE
} vga_color_t;

static inline uint8_t vga_color(vga_color_t fg, vga_color_t bg) {
    return fg | (bg << 4); // Combina as cores de primeiro plano e plano de fundo
    /*
    bg=2 (verde), fg=7 (cinza claro)
    fg:  0000 0111 
    bg:  0000 0010  →  shift 4  →  0010 0000 
    OR 0010 0111  →  0x27
     */
}

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t) c | ((uint16_t) color << 8); // Combina o caractere e a cor em um único valor de 16 bits
}

// Estado do terminal
static size_t  term_row; // Variáveis para rastrear a posição atual do cursor e a cor do texto
static size_t  term_col; // Variável para rastrear a posição atual do cursor (coluna)
static uint8_t term_color; // Variável para rastrear a cor do texto

// Fnção para habilitar o cursor do terminal, configurando os registradores do controlador de vídeo para definir a forma e a posição do cursor
static void cursor_enable(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 13); /* scanline inicial */
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15); /* scanline final */
}

// Função para atualizar a posição do cursor no terminal, calculando a posição linear com base na linha e coluna atuais e configurando os registradores do controlador de vídeo para refletir essa posição
static void cursor_update(void) {
    uint16_t pos = term_row * VGA_COLS + term_col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

// Driver de terminal
void term_init(void) {
    term_row   = 0;
    term_col   = 0;
    term_color = vga_color(VGA_LIGHT_GREEN, VGA_BLACK);

    for (size_t r = 0; r < VGA_ROWS; r++)
        for (size_t c = 0; c < VGA_COLS; c++)
            VGA_ADDR[r * VGA_COLS + c] = vga_entry(' ', term_color);
    cursor_enable();
}

// Função auxiliar para mudar a cor do texto 
void term_set_color(vga_color_t fg, vga_color_t bg) {
    term_color = vga_color(fg, bg);
}

// Função auxiliar para rolar o terminal para cima quando o cursor atinge a última linha
static void term_scroll(void) {
    /* copia cada linha para a posição anterior — linha 1 vai para 0, 2 para 1, etc. */
    for (size_t r = 1; r < VGA_ROWS; r++)
        for (size_t c = 0; c < VGA_COLS; c++)
            VGA_ADDR[(r - 1) * VGA_COLS + c] = VGA_ADDR[r * VGA_COLS + c];

    /* limpa a última linha — após o scroll ela ficaria duplicada */
    for (size_t c = 0; c < VGA_COLS; c++)
        VGA_ADDR[(VGA_ROWS - 1) * VGA_COLS + c] = vga_entry(' ', term_color);

    /* reposiciona o cursor na última linha, agora vazia */
    term_row = VGA_ROWS - 1;
}

// Função para escrever um caractere no terminal, lidando com caracteres de controle como nova linha, rowback e backspace
void term_putchar(char c) {
    // Lida com caracteres de controle: nova linha, retorno de carro e backspace
    if (c == '\n') {
        term_col = 0;
        if (++term_row == VGA_ROWS) term_scroll();
        return;
    }
    if (c == '\r') { term_col = 0; return; }
    if (c == '\b' && term_col > 0) {
        term_col--;
        VGA_ADDR[term_row * VGA_COLS + term_col] = vga_entry(' ', term_color);
        return;
    }

    // Escreve o caractere no buffer de vídeo e avança o cursor
    VGA_ADDR[term_row * VGA_COLS + term_col] = vga_entry(c, term_color);
    if (++term_col == VGA_COLS) {
        term_col = 0;
        if (++term_row == VGA_ROWS) term_scroll();
    }
    cursor_update();
}

// Função para imprimir uma string no terminal, caractere por caractere
void term_print(const char *s) {
    while (*s) term_putchar(*s++);
}

// Função para imprimir uma string seguida de uma nova linha
void term_println(const char *s) {
    term_print(s);
    term_putchar('\n');
}

// Função auxiliar para comparar duas strings, usada para implementar comandos simples no shell
static int str_eq(const char *a, const char *b) {
    while (*a && *b) // Compara os caracteres de ambas as strings até encontrar um terminador nulo
        if (*a++ != *b++) return 0; // Se os caracteres diferirem, retorna 0 (falso)
    return *a == *b;
}

// Comando simples para limpar a tela do terminal, redefinindo o cursor para a posição inicial e re-inicializando o terminal
static void cmd_clear(void) {
    term_row = 0;
    term_col = 0;
    term_init();
}

// Comando simples para exibir informações sobre comandos disponíveis, usando cores para destacar o título e os comandos
static void cmd_help(void) {
    term_set_color(VGA_CYAN, VGA_BLACK);
    term_println("Comandos disponiveis:");
    term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    term_println("  help    -- esta mensagem");
    term_println("  about   -- sobre o MF-0S");
    term_println("  clear   -- limpa a tela");
    term_println("  halt    -- desliga");
    term_println("  uptime  -- ticks desde o boot");
    term_println("  memtest -- testa o heap");
}

// Comando simples para exibir informações sobre o sistema operacional, usando cores para destacar o título e o autor
static void cmd_about(void) {
    term_println("MF-0S 'MyFucking-OS': um sistema operacional minimalista escrito em C");
    term_println("Desenvolvido para fins educacionais e de aprendizado");
    term_set_color(VGA_LIGHT_RED, VGA_BLACK);
    term_println("Feito com cafe e muita procrastinacao de tarefas importantes por Bruno Hemann");
}

// Função para criar um delay usando o timer do sistema
static void delay_ticks(uint32_t t) {
    uint32_t start = timer_ticks();
    while (timer_ticks() - start < t);
}

// Função para exibir uma splash screen com o nome do sistema operacional e mais firulas...
static void splash(void) {
    term_set_color(VGA_LIGHT_RED, VGA_BLACK);
    term_println("ooo         ooooo ooooooooooo           .oooooo.    .oooooo..o "); delay_ticks(100);
    term_println("`88.       .888' `888'    `8           d8P'  `Y8b  d8P'    `Y8 "); delay_ticks(100);
    term_println(" 888b     d'888   888                 888      888 Y88bo.       "); delay_ticks(100);
    term_println(" 8 Y88. .P  888   888oooo8            888      888  `\"Y8888o.  "); delay_ticks(100);
    term_println(" 8  `888'   888   888    \"    8888888 888      888     `\"Y88b "); delay_ticks(100);
    term_println(" 8    Y     888   888                 `88b    d88' oo     .d8P  "); delay_ticks(100);
    term_println("o8o        o888o o888o                 `Y8bood8P'  8\"\"88888P'  "); delay_ticks(100);
    term_putchar('\n');
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_println("MyFuckingOS v0.4 - Developed by Bruno Hemann"); delay_ticks(8);
    term_println("x86 32-bit kernel | Paging enabled"); delay_ticks(8);
    term_putchar('\n');
    term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

static void term_print_uint(uint32_t n) {
    if (n == 0) { term_putchar('0'); return; }
    char buf[12];
    int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    for (int j = i - 1; j >= 0; j--) term_putchar(buf[j]);
}

static void cmd_uptime(void) {
    term_print("Ticks desde o boot: ");
    term_print_uint(timer_ticks());
    term_println(" (100 ticks = 1 segundo)");
}

// Comando de teste para verificar a funcionalidade do heap, alocando dois inteiros, atribuindo valores e imprimindo-os no terminal
static void cmd_memtest(void) {
    uint32_t *a = (uint32_t*) kmalloc(sizeof(uint32_t));
    uint32_t *b = (uint32_t*) kmalloc(sizeof(uint32_t));

    *a = 42;
    *b = 58;

    term_print("a = "); term_print_uint(*a); term_putchar('\n');
    term_print("b = "); term_print_uint(*b); term_putchar('\n');
    term_print("a + b = "); term_print_uint(*a + *b); term_putchar('\n');
}

// Função principal do shell, que exibe um prompt e processa os comandos digitados pelo usuário em um loop infinito
static void shell_run(void) {
    char buf[CMD_MAX]; // Buffer para armazenar o comando digitado pelo usuário, com um tamanho máximo definido por CMD_MAX

    while (1) {
        term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        term_print("MF-0S> ");
        term_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        int len = 0;
        while (1) { // Loop para ler caracteres do teclado até que o usuário pressione Enter, armazenando-os no buffer e lidando com backspace
            char c = kb_getchar();
            if (!c) continue; // Ignora caracteres inválidos (como scancodes de liberação de tecla)

            if (c == '\n') {
                term_putchar('\n');
                buf[len] = '\0';
                break;
            }
            if (c == '\b' && len > 0) {
                len--;
                term_putchar('\b');
                continue;
            }
            if (len < CMD_MAX - 1) {
                buf[len++] = c;
                term_putchar(c);
            }
        }
        if (len == 0) continue; // Se o usuário apenas pressionou Enter sem digitar um comando, exibe o prompt novamente
        // Processa o comando digitado pelo usuário, comparando-o com os comandos disponíveis e executando a função correspondente
        if      (str_eq(buf, "help"))  cmd_help();
        else if (str_eq(buf, "about")) cmd_about();
        else if (str_eq(buf, "clear")) cmd_clear();
        else if (str_eq(buf, "halt"))  {
            term_println("Ate logo.");
            __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x00), "Nd"((uint16_t)0xf4));        }
        else if (str_eq(buf, "uptime")) cmd_uptime();
        else if (str_eq(buf, "memtest")) cmd_memtest();
        else {
            // Se o comando não for reconhecido, exibe uma mensagem de erro em vermelho
            term_set_color(VGA_LIGHT_RED, VGA_BLACK);
            term_print("Comando nao encontrado: ");
            term_println(buf);
            term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        }
    }
}

static void process_a(void) {
    while (1) {
        term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        term_print("A");
    }
}

static void process_b(void) {
    while (1) {
        term_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        term_print("B");
    }
}

// Função principal do kernel, que inicializa o terminal e inicia o shell para interagir com o usuário
// Att: A função gdt_init() é chamada para configurar a Global Descriptor Table (GDT) antes de inicializar o terminal e o shell, garantindo que o sistema esteja em um estado adequado para execução.
// Att2: A função idt_init() é chamada para configurar a Interrupt Descriptor Table (IDT) antes de inicializar o terminal e o shell, garantindo que o sistema possa lidar com interrupções corretamente.
// Att3: A função pic_init() é chamada para configurar o Programmable Interrupt Controller (PIC) antes de inicializar o terminal e o shell, garantindo que as interrupções sejam remapeadas e possam ser gerenciadas adequadamente.
// Att4: A função isr_init() é chamada para configurar as Interrupt Service Routines (ISRs) antes de inicializar o terminal e o shell, garantindo que os handlers de interrupção estejam configurados corretamente para lidar com eventos como interrupções de teclado.
// Att5: A função timer_init() é chamada para configurar o timer do sistema antes de inicializar o terminal e o shell, garantindo que o sistema possa contar o tempo e lidar com interrupções de timer corretamente.
// Att6: A função heap_init() é chamada para configurar o heap de memória antes de inicializar o terminal e o shell, garantindo que a alocação dinâmica de memória esteja disponível para o sistema e os programas que possam ser executados no futuro.
// Att7: A função paging_init() é chamada para configurar o sistema de paginação antes de inicializar o terminal e o shell, garantindo que a memória virtual esteja habilitada e configurada corretamente para o sistema operacional.
// Att8: A função process_init() é chamada para inicializar o gerenciador de processos, preparando as estruturas de dados necessárias para criar e alternar entre processos.
void kernel_main(void) {
    gdt_init();
    idt_init();
    pic_init();
    isr_init();
    timer_init();
    heap_init();
    paging_init();
    process_init();
    term_init();
    __asm__ volatile ("sti");
    splash();
    process_create(process_a);
    process_create(process_b);
    shell_run();
}