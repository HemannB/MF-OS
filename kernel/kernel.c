#include <stdint.h>
#include <stddef.h>
#include "gdt.h"
#include "idt.h"

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

// Driver de terminal
void term_init(void) {
    term_row   = 0;
    term_col   = 0;
    term_color = vga_color(VGA_LIGHT_GREEN, VGA_BLACK);

    for (size_t r = 0; r < VGA_ROWS; r++)
        for (size_t c = 0; c < VGA_COLS; c++)
            VGA_ADDR[r * VGA_COLS + c] = vga_entry(' ', term_color);
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

// Função para ler um byte de uma porta de E/S usando a instrução inb do x86
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile (
        "inb %1, %0"        // lê da porta %1 para o registrador %0
        : "=a"(val)       // output: variável 'val' recebe registrador AL (parte do EAX)
        : "Nd"(port)        // input: 'port' vai para DX (registrador D)
    );
    return val;
}

// Tabela simplificada de conversão do scancode da tecla para um caracter ACII (sem considerar telcas de escape como Shift, Ctrl, etc.)
static const char sc_ascii[] = {
    0,  0,  '1','2','3','4','5','6','7','8','9','0','-','=', '\b',  0,
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',  0, '\\',
    'z','x','c','v','b','n','m',',','.','/',  0,  '*', 0, ' '
};

// Função para ler um caracter do teclado usando o controlador de teclado do PC (porta 0x60 para dados e 0x64 para status)
char kb_getchar(void) {
    uint8_t sc;
    do { sc = inb(0x64); } while (!(sc & 0x01)); // Espera até que haja um scancode disponível (bit 0 do status indica isso)
    sc = inb(0x60); // Lê o scancode da porta de dados do teclado
    if (sc & 0x80) return 0; // Ignora scancodes de liberação de tecla (bit 7 indica isso)
    if (sc < sizeof(sc_ascii)) return sc_ascii[sc]; // Converte o scancode para um caracter ASCII usando a tabela de conversão
    return 0;
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
}

// Comando simples para exibir informações sobre o sistema operacional, usando cores para destacar o título e o autor
static void cmd_about(void) {
    term_println("MF-0S 'MyFucking-OS': um sistema operacional minimalista escrito em C");
    term_println("Desenvolvido para fins educacionais e de aprendizado");
    term_set_color(VGA_LIGHT_RED, VGA_BLACK);
    term_println("Feito com cafe e muita procrastinacao de tarefas importantes por Bruno Hemann");
}

// Função principal do shell, que exibe um prompt e processa os comandos digitados pelo usuário em um loop infinito
static void shell_run(void) {
    char buf[CMD_MAX]; // Buffer para armazenar o comando digitado pelo usuário, com um tamanho máximo definido por CMD_MAX

    while (1) {
        term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        term_print("MF-0S> ");
        term_set_color(VGA_WHITE, VGA_BLACK);
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
            __asm__ volatile ("hlt");
        }
        else {
            // Se o comando não for reconhecido, exibe uma mensagem de erro em vermelho
            term_set_color(VGA_LIGHT_RED, VGA_BLACK);
            term_print("Comando nao encontrado: ");
            term_println(buf);
            term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        }
    }
}

// Função principal do kernel, que inicializa o terminal e inicia o shell para interagir com o usuário
// Att: A função gdt_init() é chamada para configurar a Global Descriptor Table (GDT) antes de inicializar o terminal e o shell, garantindo que o sistema esteja em um estado adequado para execução.
// Att2: A função idt_init() é chamada para configurar a Interrupt Descriptor Table (IDT) antes de inicializar o terminal e o shell, garantindo que o sistema possa lidar com interrupções corretamente.
void kernel_main(void) {
    gdt_init();
    idt_init();
    term_init();
    shell_run();
}