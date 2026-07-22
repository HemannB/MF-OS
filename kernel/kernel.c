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
#define MFOS_NAME "MF-0S"
#define MFOS_VERSION "0.9"
#define MFOS_EDITION "Doom Edition"

static int str_eq(const char *a, const char *b) {
    while (*a && *b)
        if (*a++ != *b++) return 0;
    return *a == *b;
}

static void delay_ticks(uint32_t t) {
    uint32_t start = timer_ticks();
    while (timer_ticks() - start < t);
}

static void boot_status(const char *component, const char *detail) {
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_print("  [ ");
    term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    term_print("OK");
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_print(" ] ");
    term_set_color(VGA_WHITE, VGA_BLACK);
    term_print(component);
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_print("  ");
    term_println(detail);
    delay_ticks(4);
}

static void print_hex32(uint32_t value) {
    char hex[9];
    hex[8] = '\0';
    for (int i = 0; i < 8; i++) {
        hex[7 - i] = "0123456789ABCDEF"[value & 0xF];
        value >>= 4;
    }
    term_print(hex);
}

static void splash(void) {
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_println("  ==========================================================================  ");
    term_set_color(VGA_LIGHT_RED, VGA_BLACK);
    term_println("   __  __ _____       ___  ____  ");
    term_println("  |  \\/  |  ___|     / _ \\/ ___| ");
    term_println("  | |\\/| | |_  _____| | | \\___ \\ ");
    term_println("  |_|  |_|_|          \\___/|____/ ");
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_print("  MyFuckingOS  /  v");
    term_print(MFOS_VERSION);
    term_print("  /  x86 bare metal  /  ");
    term_println(MFOS_EDITION);
    term_println("  --------------------------------------------------------------------------  ");
    term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    term_println("  BOOT SEQUENCE");
    term_putchar('\n');
}

static void boot_summary(uint32_t mapped_bytes, uint32_t heap_base,
                         uint32_t heap_size) {
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_println("  --------------------------------------------------------------------------  ");
    term_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    term_print("  MEMORY  ");
    term_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    term_print_uint(mapped_bytes / (1024 * 1024));
    term_print("MB mapped  /  ");
    term_print_uint(heap_size / (1024 * 1024));
    term_print("MB heap @ 0x");
    print_hex32(heap_base);
    term_putchar('\n');
    term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    term_print("  READY   ");
    term_set_color(VGA_DARK_GREY, VGA_BLACK);
    term_println("type 'help' for commands or 'doom' to launch the game");
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
    term_print(MFOS_NAME);
    term_println(" 'MyFucking-OS': kernel x86 32-bit escrito do zero em C e Assembly");
    term_set_color(VGA_LIGHT_RED, VGA_BLACK);
    term_println("Feito com cafe e muita procrastinacao por Bruno Hemann");
}

static void cmd_clear(void) { term_init(); }

static void cmd_version(void) {
    term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    term_print(MFOS_NAME);
    term_print(" v");
    term_print(MFOS_VERSION);
    term_print(" - x86 32-bit kernel | ");
    term_println(MFOS_EDITION);
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
        term_print("mf0s");
        term_set_color(VGA_DARK_GREY, VGA_BLACK);
        term_print(":");
        term_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        term_print("/");
        term_set_color(VGA_WHITE, VGA_BLACK);
        term_print("# ");
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
    boot_status("GDT", "kernel segments loaded");
    boot_status("IDT", "interrupt table ready");
    boot_status("PIC", "IRQs remapped to 0x20-0x2F");
    boot_status("RAMDISK", "Multiboot modules mounted");
    boot_status("PAGING", "identity map and linear framebuffer active");
    boot_status("PIT", "100Hz timer and preemption ready");
    boot_status("INPUT", "PS/2 keyboard listening on IRQ1");
    boot_summary(mapped_bytes, heap_base, heap_size);
    shell_run();
}
