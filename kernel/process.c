#include "process.h"
#include "heap.h"

static process_t processes[MAX_PROCESSES]; // array com todos os processos
static int       process_count = 0; // quantos processos existem atualmente
static int       current_pid   = 0; // indice do processo atualmente em execução

// Função para incializar o sistema de processos
void process_init(void) {
    // Inicializa o array de processos, definindo o PID de cada processo como 0 e seu estado como bloqueado, indicando que eles não estão prontos para execução até que sejam criados e configurados adequadamente
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].pid   = 0;
        processes[i].state = PROCESS_BLOCKED;
    }
}

// Função para criar um novo processo, recebendo um ponteiro de função que representa o ponto de entrada do processo a ser criado, alocando um novo processo, inicializando seu estado e registradores, e o adicionando à lista de processos prontos para execução, retornando o identificador do processo criado ou -1 em caso de falha
int process_create(void (*entry)(void)) {
    if (process_count >= MAX_PROCESSES) return -1;

    int i = process_count++;
    processes[i].pid         = i;
    processes[i].state       = PROCESS_READY;
    processes[i].regs.eip    = (uint32_t) entry;
    processes[i].regs.esp    = (uint32_t) &processes[i].stack[STACK_SIZE - 1];
    processes[i].regs.eflags = 0x202; // EFLAGS com bit IF (Interrupt Flag) setado para permitir interrupções

    return i;
}

void schedule(void) {
    if (process_count == 0) return;

    // marca o processo atual como READY
    processes[current_pid].state = PROCESS_READY;

    // próximo processo — round-robin 
    current_pid = (current_pid + 1) % process_count;

    // marca o próximo como RUNNING
    processes[current_pid].state = PROCESS_RUNNING;
}

// Função para obter o processo atual
process_t* process_current(void) {
    return &processes[current_pid];
}