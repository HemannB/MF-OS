#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define STACK_SIZE 4096  // Tamanho da pilha para cada processo, definido como 4KB, o que é suficiente para a maioria dos processos simples e ajuda a evitar stack overflow
#define MAX_PROCESSES 8 // Número máximo de processos que o sistema pode gerenciar simultaneamente, definido como 8 para manter a simplicidade do sistema de agendamento

// ENUM para representar o estado de um processo, indicando se ele está pronto para ser executado, atualmente em execução ou bloqueado aguardando algum evento
typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED
} process_state_t;

// STRUCT para representar registradores que serão salvos durante o Context Switch, contendo os registradores de uso geral (eax, ebx, ecx, edx), os registradores de pilha (esp, ebp) e os registradores de índice (esi, edi), além do registrador de instrução (eip) e o registrador de flags (eflags)
typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esp, ebp, esi, edi;
    uint32_t eip, eflags;
} registers_t;

// STRUCT para representar um processo, contendo um identificador de processo (pid), o estado atual do processo (state), os registradores salvos para o Context Switch (regs) e uma pilha de execução (stack) para o processo
typedef struct {
    uint32_t        pid;
    process_state_t state;
    registers_t     regs;
    uint8_t         stack[STACK_SIZE];
} process_t;


void     process_init(void); // Função para inicializar o sistema de processos, configurando as estruturas de dados necessárias para gerenciar os processos e preparando o sistema para criar e agendar processos
int      process_create(void (*entry)(void)); //  recebe um ponteiro de função que representa o ponto de entrada do processo a ser criado, aloca um novo processo, inicializa seu estado e registradores, e o adiciona à lista de processos prontos para execução, retornando o identificador do processo criado ou -1 em caso de falha
void     schedule(void); // é chamado pelo timer a cada tick para decidir quem roda.  
process_t* process_current(void); 
void     context_switch(registers_t *old, registers_t *new);
#endif