#include "tests.h"
#include "heap.h"
#include "vga13h.h"
#include "process.h"
#include "timer.h"
#include "terminal.h"

/* testa o heap alocando dois inteiros e verificando operações básicas */
void cmd_memtest(void) {
    uint32_t *a = (uint32_t*) kmalloc(sizeof(uint32_t));
    uint32_t *b = (uint32_t*) kmalloc(sizeof(uint32_t));
    *a = 42;
    *b = 58;
    term_print("a = "); term_print_uint(*a); term_putchar('\n');
    term_print("b = "); term_print_uint(*b); term_putchar('\n');
    term_print("a + b = "); term_print_uint(*a + *b); term_putchar('\n');
}

/* alias para cmd_memtest — nome canônico de teste */
void test_heap(void) {
    cmd_memtest();
}

/* testa scheduler preemptivo — dois processos alternando sem yield */
void test_scheduler(void) {
    process_create(process_a);
    process_create(process_b);
    process_run();
}

/* testa VGA modo 13h com gradiente de cores */
void test_vga13h(void) {
    vga_init_mode13h();
    for (int y = 0; y < 200; y++)
        for (int x = 0; x < 320; x++)
            vga_put_pixel(x, y, (x + y) % 256);
    vga_swap();
    uint32_t start = timer_ticks();
    while (timer_ticks() - start < 300);
}

/* processos de teste para o scheduler */
void process_a(void) {
    while (1) {
        term_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        term_print("A");
    }
}

void process_b(void) {
    while (1) {
        term_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        term_print("B");
    }
}