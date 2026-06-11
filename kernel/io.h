#ifndef IO_H
#define IO_H

#include <stdint.h>

// Escreve um byte em uma porta de E/S, usada para comunicação direta com dispositivos de hardware como controladores de vídeo, teclado e PIC
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Lê um byte de uma porta de E/S, usada para obter o estado de dispositivos de hardware como o controlador de teclado e registradores VGA
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

#endif