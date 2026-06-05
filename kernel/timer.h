#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_init(void); // Inicializa o timer
uint32_t timer_ticks(void); // Retorna o número de ticks desde a inicialização do timer

#endif