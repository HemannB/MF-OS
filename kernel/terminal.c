#include "terminal.h"
#include "io.h"
#include <stddef.h>

// Endereço do buffer de vídeo VGA em modo texto — escrever aqui exibe caracteres na tela diretamente
#define VGA_ADDR  ((volatile uint16_t*) 0xB8000)
#define VGA_COLS  80  // número de colunas do terminal
#define VGA_ROWS  25  // número de linhas do terminal

// Estado interno do terminal — rastreiam a posição atual do cursor e a cor do texto
static size_t  term_row;
static size_t  term_col;
static uint8_t term_color;

// Combina as cores de primeiro plano e plano de fundo em um único byte de atributo VGA
// bg=2 (verde), fg=7 (cinza claro)
// fg:  0000 0111
// bg:  0000 0010 → shift 4 → 0010 0000
//                  OR         0010 0111 → 0x27
static inline uint8_t vga_color(vga_color_t fg, vga_color_t bg) {
    return fg | (bg << 4);
}

// Combina o caractere e o atributo de cor em uma célula VGA de 16 bits
static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t) c | ((uint16_t) color << 8);
}

// Habilita o cursor piscante do hardware VGA, configurando as scanlines inicial e final
// via registradores do controlador de vídeo (portas 0x3D4/0x3D5)
static void cursor_enable(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 13); /* scanline inicial */
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15); /* scanline final */
}

// Atualiza a posição do cursor de hardware para refletir a posição atual do terminal
// A posição linear é calculada como: pos = linha * 80 + coluna
static void cursor_update(void) {
    uint16_t pos = term_row * VGA_COLS + term_col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));        /* byte baixo da posição */
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF)); /* byte alto da posição */
}

// Rola o terminal uma linha para cima quando o cursor ultrapassa a última linha
// Copia cada linha para a posição anterior e limpa a última linha
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

// Inicializa o terminal — zera o cursor, define a cor padrão e limpa toda a tela
void term_init(void) {
    term_row   = 0;
    term_col   = 0;
    term_color = vga_color(VGA_LIGHT_GREEN, VGA_BLACK);

    for (size_t r = 0; r < VGA_ROWS; r++)
        for (size_t c = 0; c < VGA_COLS; c++)
            VGA_ADDR[r * VGA_COLS + c] = vga_entry(' ', term_color);

    cursor_enable();
}

// Altera a cor do texto e do fundo para os próximos caracteres escritos no terminal
void term_set_color(vga_color_t fg, vga_color_t bg) {
    term_color = vga_color(fg, bg);
}

// Escreve um único caractere no terminal, tratando os caracteres de controle:
// \n → nova linha, \r → retorno ao início da linha, \b → backspace
void term_putchar(char c) {
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

    /* escreve o caractere no buffer de vídeo e avança o cursor */
    VGA_ADDR[term_row * VGA_COLS + term_col] = vga_entry(c, term_color);
    if (++term_col == VGA_COLS) {
        term_col = 0;
        if (++term_row == VGA_ROWS) term_scroll();
    }
    cursor_update();
}

// Imprime uma string no terminal caractere por caractere até encontrar o terminador nulo
void term_print(const char *s) {
    while (*s) term_putchar(*s++);
}

// Imprime uma string seguida de uma quebra de linha
void term_println(const char *s) {
    term_print(s);
    term_putchar('\n');
}

// Converte e imprime um número inteiro sem sinal em decimal
// Sem stdlib — converte dígito por dígito usando divisão e módulo
void term_print_uint(uint32_t n) {
    if (n == 0) { term_putchar('0'); return; }
    char buf[12];
    int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    for (int j = i - 1; j >= 0; j--) term_putchar(buf[j]);
}