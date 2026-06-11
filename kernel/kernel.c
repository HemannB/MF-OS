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
#include "vga13h.h"
#include "terminal.h"
#include "tests.h"

#define CMD_MAX 128 // Tamanho máximo do buffer de comando do shell

// Compara duas strings caractere por caractere — substitui strcmp da stdlib que não está disponível em freestanding
static int str_eq(const char *a, const char *b) {
    while (*a && *b)
        if (*a++ != *b++) return 0;
    return *a == *b;
}

// Cria um delay bloqueante usando o contador de ticks do timer (100 ticks = 1 segundo)
static void delay_ticks(uint32_t t) {
    uint32_t start = timer_ticks();
    while (timer_ticks() - start < t);
}

// Exibe a splash screen animada na inicialização — cada linha aparece com delay de 1 segundo
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
    term_println("MyFuckingOS v0.5 - Developed by Bruno Hemann"); delay_ticks(8);
    term_println("x86 32-bit kernel | Paging enabled"); delay_ticks(8);
    term_putchar('\n');
    term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

// Exibe a lista de comandos disponíveis com destaque de cor para o título
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
    term_println("  version -- versao do sistema");
}

// Exibe informações sobre o MF-0S e seu autor
static void cmd_about(void) {
    term_println("MF-0S 'MyFucking-OS': um sistema operacional minimalista escrito em C");
    term_println("Desenvolvido para fins educacionais e de aprendizado");
    term_set_color(VGA_LIGHT_RED, VGA_BLACK);
    term_println("Feito com cafe e muita procrastinacao de tarefas importantes por Bruno Hemann");
}

// Limpa a tela reinicializando o terminal e exibe uma mensagem de confirmação
static void cmd_clear(void) {
    term_init();
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_println("Tela limpa.");
}

// Exibe a versão atual do sistema operacional
static void cmd_version(void) {
    term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    term_println("MF-0S v0.5 - x86 32-bit kernel");
    term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

// Exibe o número de ticks desde o boot — 100 ticks equivalem a 1 segundo
static void cmd_uptime(void) {
    term_print("Ticks desde o boot: ");
    term_print_uint(timer_ticks());
    term_println(" (100 ticks = 1 segundo)");
}

// Loop principal do shell — exibe o prompt, lê comandos do teclado e despacha para os handlers
static void shell_run(void) {
    char buf[CMD_MAX];

    while (1) {
        /* exibe o prompt com cores distintas para nome e símbolo */
        term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        term_print("MF-0S");
        term_set_color(VGA_WHITE, VGA_BLACK);
        term_print("> ");
        term_set_color(VGA_LIGHT_CYAN, VGA_BLACK);

        /* lê caracteres até Enter, tratando backspace e limite do buffer */
        int len = 0;
        while (1) {
            char c = kb_getchar();
            if (!c) continue; /* ignora scancodes sem ASCII (Shift, Ctrl, F1...) */

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

        if (len == 0) continue; /* Enter sem comando — exibe o prompt novamente */

        /* despacha o comando para o handler correspondente */
        if      (str_eq(buf, "help"))    cmd_help();
        else if (str_eq(buf, "about"))   cmd_about();
        else if (str_eq(buf, "clear"))   cmd_clear();
        else if (str_eq(buf, "uptime"))  cmd_uptime();
        else if (str_eq(buf, "memtest")) cmd_memtest();
        else if (str_eq(buf, "version")) cmd_version();
        else if (str_eq(buf, "halt")) {
            term_println("Ate logo.");
            __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x00), "Nd"((uint16_t)0xf4));
        }
        else {
            term_set_color(VGA_LIGHT_RED, VGA_BLACK);
            term_print("Comando nao encontrado: ");
            term_println(buf);
            term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        }
    }
}

// Ponto de entrada do kernel — inicializa todos os subsistemas em ordem e inicia o shell
// A ordem importa: GDT antes de IDT, IDT antes de PIC, PIC antes de ISR/timer
void kernel_main(void) {
    gdt_init();       /* configura a Global Descriptor Table */
    idt_init();       /* configura a Interrupt Descriptor Table */
    pic_init();       /* remapeia IRQs do PIC para 0x20-0x2F */
    isr_init();       /* registra handler do teclado (IRQ1) */
    timer_init();     /* configura PIT a 100Hz (IRQ0) */
    heap_init();      /* inicializa o heap a partir de 2MB */
    paging_init();    /* ativa paginação com identity mapping de 4MB */
    process_init();   /* inicializa o gerenciador de processos */
    term_init();      /* inicializa o driver de terminal VGA */
    __asm__ volatile ("sti"); /* habilita interrupções */
    splash();
    shell_run();
}