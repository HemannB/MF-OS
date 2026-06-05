#include "idt.h"

static idt_entry_t idt[256];
static idt_ptr_t   idt_ptr;

void idt_set_entry(uint8_t n, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[n].base_low  = (base & 0xFFFF);
    idt[n].base_high = (base >> 16) & 0xFFFF;
    idt[n].selector  = selector;
    idt[n].zero      = 0;
    idt[n].flags     = flags;
}

void idt_init(void) {
    idt_ptr.limit = (sizeof(idt_entry_t) * 256) - 1;
    idt_ptr.base  = (uint32_t) &idt;

    idt_flush((uint32_t) &idt_ptr);
}

