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
#include "libc.h"
#include "multiboot.h"

#define CMD_MAX 128

static int str_eq(const char *a, const char *b) {
    while (*a && *b)
        if (*a++ != *b++) return 0;
    return *a == *b;
}

static void delay_ticks(uint32_t t) {
    uint32_t start = timer_ticks();
    while (timer_ticks() - start < t);
}

static void boot_msg(const char *msg) {
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_print("  [");
    term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    term_print("OK");
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_print("] ");
    term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    term_println(msg);
    delay_ticks(10);
}

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
    term_println("  MyFuckingOS v0.9 - Developed by Bruno Hemann");
    term_println("  x86 32-bit kernel | Doom Edition");
    term_putchar('\n');
}

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
    term_println("  schedtest -- testa preempcao round-robin");
    term_println("  version -- versao do sistema");
    term_println("  ls      -- lista arquivos no ramdisk");
    term_println("  cat     -- exibe conteudo de um arquivo");
    term_println("  vgatest -- testa VGA direto no LFB");
    term_println("  doom    -- inicia o Doom");
}

static void cmd_about(void) {
    term_println("MF-0S 'MyFucking-OS': kernel x86 32-bit escrito do zero em C e Assembly");
    term_set_color(VGA_LIGHT_RED, VGA_BLACK);
    term_println("Feito com cafe e muita procrastinacao por Bruno Hemann");
}

static void cmd_clear(void) { term_init(); }

static void cmd_version(void) {
    term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    term_println("MF-0S v0.9 - x86 32-bit kernel | Doom Edition");
    term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

static void cmd_uptime(void) {
    term_print("Ticks: ");
    term_print_uint(timer_ticks());
    term_println(" (100 ticks = 1s)");
}

static void cmd_ls(void) {
    term_set_color(VGA_CYAN, VGA_BLACK);
    term_println("Arquivos no ramdisk:");
    term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
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

/* testa VBE escrevendo gradiente direto no LFB */
static void cmd_vgatest(void) {
    term_println("Iniciando VGA test...");
    delay_ticks(20);
    vga_init_mode13h();
    terminal_set_graphics(1);
    vga_test();
    /* aguarda 3 segundos */
    uint32_t start = timer_ticks();
    while (timer_ticks() - start < 300);
    terminal_set_graphics(0);
    term_init();
    term_println("VGA test concluido.");
}

extern void doom_main(void);

static void cmd_doom(void) {
    fs_file_t *wad = fs_open("doom1.wad");
    if (!wad) {
        term_set_color(VGA_LIGHT_RED, VGA_BLACK);
        term_println("ERRO: doom1.wad nao encontrado no ramdisk!");
        term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        return;
    }
    term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    term_println("Iniciando Doom...");
    delay_ticks(20);
    doom_main();
    terminal_set_graphics(0);
    term_init();
}

static void shell_run(void) {
    char buf[CMD_MAX];
    while (1) {
        term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        term_print("MF-0S");
        term_set_color(VGA_WHITE, VGA_BLACK);
        term_print("> ");
        term_set_color(VGA_LIGHT_CYAN, VGA_BLACK);

        int len = 0;
        while (1) {
            char c = kb_getchar();
            if (!c) continue;
            if (c == '\n') { term_putchar('\n'); buf[len] = '\0'; break; }
            if (c == '\b' && len > 0) { len--; term_putchar('\b'); continue; }
            if (len < CMD_MAX - 1) { buf[len++] = c; term_putchar(c); }
        }

        if (len == 0) continue;

        if      (str_eq(buf, "help"))    cmd_help();
        else if (str_eq(buf, "about"))   cmd_about();
        else if (str_eq(buf, "clear"))   cmd_clear();
        else if (str_eq(buf, "uptime"))  cmd_uptime();
        else if (str_eq(buf, "memtest")) cmd_memtest();
        else if (str_eq(buf, "schedtest")) test_scheduler();
        else if (str_eq(buf, "version")) cmd_version();
        else if (str_eq(buf, "ls"))      cmd_ls();
        else if (str_eq(buf, "vgatest")) cmd_vgatest();
        else if (str_eq(buf, "doom"))    cmd_doom();
        else if (buf[0]=='c' && buf[1]=='a' && buf[2]=='t' && buf[3]==' ')
            cmd_cat(buf + 4);
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

void kernel_main(uint32_t multiboot_info_addr) {
    multiboot_info_t *boot_info = (multiboot_info_t*)multiboot_info_addr;
    term_init();

    gdt_init();
    idt_init();
    pic_init();

    fs_init(multiboot_info_addr);
    uint32_t memory_bytes = multiboot_memory_bytes(boot_info);
    uint32_t mapped_bytes = paging_init(memory_bytes);
    uint32_t heap_base = fs_heap_base();
    uint32_t heap_size = heap_base < mapped_bytes ? mapped_bytes - heap_base : 0;
    heap_init(heap_base, heap_size);

    process_init();

    isr_init();
    timer_init();
    __asm__ volatile ("sti");

    splash();
    boot_msg("GDT carregada");
    boot_msg("IDT configurada");
    boot_msg("PIC remapeado (IRQs 0x20-0x2F)");
    boot_msg("Sistema de arquivos pronto");

    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_print("  [");
    term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    term_print("OK");
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_print("] ");
    term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    term_print("Heap em 0x");
    uint32_t base = heap_base;
    char hex[9]; hex[8] = '\0';
    for (int i = 0; i < 8; i++) { hex[7-i] = "0123456789ABCDEF"[base & 0xF]; base >>= 4; }
    term_print(hex);
    term_print(" (+");
    term_print_uint(heap_size / (1024 * 1024));
    term_println("MB)");

    boot_msg("Paginacao ativa (limite Multiboot + LFB 0xFD000000)");
    boot_msg("Timer PIT a 100Hz (preempcao sob demanda)");
    boot_msg("Teclado IRQ1 pronto");

    term_putchar('\n');
    shell_run();
}
