#include <stdint.h>
#include <stddef.h>

// Definições para o driver de terminal VGA
#define VGA_ADDR  ((volatile uint16_t*) 0xB8000) // Buffer de vídeo para texto 
#define VGA_COLS  80 // Número de colunas 
#define VGA_ROWS  25 // Número de linhas

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

