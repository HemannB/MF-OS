#include "process.h"
#include "heap.h"

static process_t processes[MAX_PROCESSES]; /* array com todos os processos do sistema */
static int       process_count = 0;        /* quantidade de processos criados */
static int       current_pid   = 0;        /* índice do processo em execução agora */

/* inicializa todos os slots como bloqueados — nenhum processo existe ainda */
void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].pid   = 0;
        processes[i].state = PROCESS_BLOCKED;
    }
}

/* cria um novo processo — configura a stack com o entry point no topo
   o primeiro 'ret' do context_switch vai pular para entry */
int process_create(void (*entry)(void)) {
    if (process_count >= MAX_PROCESSES) return -1;

    int i = process_count++;
    processes[i].pid   = i;
    processes[i].state = PROCESS_READY;

    uint32_t *stack_top = (uint32_t*)(&processes[i].stack[STACK_SIZE]);
    stack_top--; *stack_top = (uint32_t) entry; /* endereço de entrada — ret pula aqui */
    stack_top--; *stack_top = 0;                /* dummy ebp */
    stack_top--; *stack_top = 0;                /* dummy ebx */
    stack_top--; *stack_top = 0;                /* dummy esi */
    stack_top--; *stack_top = 0;                /* dummy edi — ESP aponta aqui */
    processes[i].esp = (uint32_t) stack_top;

    return i;
}

/* round-robin: marca atual como READY e avança para o próximo */
void schedule(void) {
    if (process_count == 0) return;
    processes[current_pid].state = PROCESS_READY;
    current_pid = (current_pid + 1) % process_count;
    processes[current_pid].state = PROCESS_RUNNING;
}

/* retorna o processo atualmente em execução */
process_t* process_current(void) {
    return &processes[current_pid];
}

/* inicia o primeiro processo — remove o entry da stack e pula para ele */
void process_run(void) {
    current_pid = 0;
    processes[0].state = PROCESS_RUNNING;

    uint32_t dummy_esp;
    context_switch(&dummy_esp, &processes[0].esp);
}

/* cede a CPU voluntariamente — salva estado atual e restaura o próximo */
void yield(void) {
    __asm__ volatile ("cli");           /* desabilita interrupções durante o switch */
    process_t *old = process_current();
    schedule();
    process_t *new = process_current();
    if (old != new)
        context_switch(&old->esp, &new->esp); /* passa ponteiros para os ESPs */
    __asm__ volatile ("sti");           /* reabilita interrupções */
}