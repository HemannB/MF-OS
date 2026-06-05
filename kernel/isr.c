#include "isr.h"
#include "idt.h"
#include "pic.h"

#define KB_BUFFER_SIZE 256

void term_putchar(char c); // Declaração da função para escrever um caractere no terminal, que será usada no handler de interrupção do teclado

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static const char sc_ascii[] = {
    0,  0,  '1','2','3','4','5','6','7','8','9','0','-','=','\b', 0,
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',  0, '\\',
    'z','x','c','v','b','n','m',',','.','/',  0,  '*', 0, ' '
};

static char     kb_buffer[KB_BUFFER_SIZE];
static volatile uint8_t kb_head = 0;
static volatile uint8_t kb_tail = 0;

// Handler para a IRQ1 (teclado), que será chamado quando uma interrupção de teclado ocorrer, lendo o scancode da tecla pressionada e processando-a para interagir com o terminal
void irq1_handler(void) {
    uint8_t sc = inb(0x60);

    if (!(sc & 0x80) && sc < sizeof(sc_ascii)) {
        char c = sc_ascii[sc];
        if (c) {
            kb_buffer[kb_head] = c;
            kb_head = (kb_head + 1) % KB_BUFFER_SIZE;
        }
    }

    pic_eoi(1);
}

char kb_getchar(void) {
    while (kb_head == kb_tail);  /* espera até ter algo no buffer */
    char c = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
    return c;
}

// Função para inicializar as ISRs (Interrupt Service Routines) e configurar a IDT (Interrupt Descriptor Table) com os handlers de interrupção apropriados
void isr_init(void) {
    extern void irq1_wrapper(void);
    idt_set_entry(33, (uint32_t) irq1_wrapper, 0x08, 0x8E);

    /* habilita apenas IRQ1 (teclado) no Master PIC */
    outb(0x21, 0xFD);  /* 0xFD = 1111 1101 — bit 1 = 0 = IRQ1 habilitada */
}