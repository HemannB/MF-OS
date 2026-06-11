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
#include "fs.h"

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

// Exibe uma mensagem de boot com prefixo [OK] colorido — indica subsistema inicializado com sucesso
static void boot_msg(const char *msg) {
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_print("  [");
    term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    term_print("OK");
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_print("] ");
    term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    term_println(msg);
    delay_ticks(15); /* pequeno delay para efeito visual de boot progressivo */
}

// Exibe a splash screen animada com o logo ASCII art e informações do sistema
static void splash(void) {
    term_set_color(VGA_LIGHT_RED, VGA_BLACK);
    term_println("ooo         ooooo ooooooooooo           .oooooo.    .oooooo..o ");
    term_println("`88.       .888' `888'    `8           d8P'  `Y8b  d8P'    `Y8 ");
    term_println(" 888b     d'888   888                 888      888 Y88bo.       ");
    term_println(" 8 Y88. .P  888   888oooo8            888      888  `\"Y8888o.  ");
    term_println(" 8  `888'   888   888    \"    8888888 888      888     `\"Y88b ");
    term_println(" 8    Y     888   888                 `88b    d88' oo     .d8P  ");
    term_println("o8o        o888o o888o                 `Y8bood8P'  8\"\"88888P'  ");
    term_putchar('\n');
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_println("  MyFuckingOS v0.8 - Developed by Bruno Hemann");
    term_println("  x86 32-bit kernel");
    term_putchar('\n');
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
    term_println("  ls      -- lista arquivos no ramdisk");
    term_println("  cat     -- exibe conteudo de um arquivo");
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
    term_println("MF-0S v0.8 - x86 32-bit kernel");
    term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

// Exibe o número de ticks desde o boot — 100 ticks equivalem a 1 segundo
static void cmd_uptime(void) {
    term_print("Ticks desde o boot: ");
    term_print_uint(timer_ticks());
    term_println(" (100 ticks = 1 segundo)");
}

// lista todos os arquivos carregados pelo GRUB no ramdisk
static void cmd_ls(void) {
    term_set_color(VGA_CYAN, VGA_BLACK);
    term_println("Arquivos no ramdisk:");
    term_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    extern fs_file_t *fs_get_file(int index);
    extern int        fs_count(void);

    int count = fs_count();
    if (count == 0) {
        term_set_color(VGA_DARK_GREY, VGA_BLACK);
        term_println("  (nenhum arquivo carregado)");
        return;
    }

    for (int i = 0; i < count; i++) {
        fs_file_t *f = fs_get_file(i);
        term_print("  ");
        term_print(f->name);
        term_print("  (");
        term_print_uint(f->size);
        term_println(" bytes)");
    }
}

// exibe o conteúdo de um arquivo do ramdisk
static void cmd_cat(const char *name) {
    fs_file_t *f = fs_open(name);
    if (!f) {
        term_set_color(VGA_LIGHT_RED, VGA_BLACK);
        term_print("Arquivo nao encontrado: ");
        term_println(name);
        term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        return;
    }

    term_set_color(VGA_WHITE, VGA_BLACK);
    for (uint32_t i = 0; i < f->size; i++)
        term_putchar((char) f->data[i]);
    term_putchar('\n');
    term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
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
        else if (str_eq(buf, "ls"))      cmd_ls();
        else if (buf[0]=='c' && buf[1]=='a' && buf[2]=='t' && buf[3]==' ') cmd_cat(buf + 4);
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

// Ponto de entrada do kernel inicializa subsistemas em ordem com feedback visual de boot
// A ordem importa: terminal primeiro para mostrar mensagens, GDT antes de IDT,
// IDT antes de PIC, PIC antes de ISR e timer, heap antes de paging e processos
void kernel_main(uint32_t multiboot_info_addr) {
    term_init();

    /* inicialização silenciosa — timer ainda não está ativo */
    gdt_init();
    idt_init();
    pic_init();
    isr_init();
    timer_init();
    __asm__ volatile ("sti"); /* habilita interrupções — delay_ticks passa a funcionar */

    /* agora o timer está ativo — splash com animação e boot messages com delay */
    splash();

    boot_msg("GDT carregada");
    boot_msg("IDT configurada");
    boot_msg("PIC remapeado (IRQs 0x20-0x2F)");
    boot_msg("ISR: teclado (IRQ1) registrado");
    boot_msg("Timer PIT a 100Hz (IRQ0)");

    heap_init();    boot_msg("Heap inicializado (2MB-3MB)");
    paging_init();  boot_msg("Paginacao ativa (4MB identity map)");
    process_init(); boot_msg("Gerenciador de processos pronto");
    fs_init(multiboot_info_addr); boot_msg("Sistema de arquivos pronto");
    
    term_putchar('\n');
    shell_run();
}