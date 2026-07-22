#include "process.h"
#include "heap.h"

static process_t processes[MAX_PROCESSES]; /* array com todos os processos do sistema */
static int       process_count = 0;        /* quantidade de processos criados */
static int       current_pid   = 0;        /* índice do processo em execução agora */
static int       scheduler_running = 0;

/* inicializa todos os slots como bloqueados — nenhum processo existe ainda */
void process_init(void) {
    process_count = 0;
    current_pid = 0;
    scheduler_running = 0;
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

    stack_top--; *stack_top = 0x202;              // EFLAGS: IF habilitado
    stack_top--; *stack_top = 0x08;               // CS: code segment
    stack_top--; *stack_top = (uint32_t) entry;   // EIP: entry point

    // pusha dummy 8 registradores
    for (int j = 0; j < 8; j++) {
        stack_top--;
        *stack_top = 0;
    }

    processes[i].esp = (uint32_t) stack_top;
    return i;
}

// round-robin: marca atual como READY e avança para o próximo
void schedule(void) {
    if (process_count == 0) return;

    int previous = current_pid;
    if (processes[previous].state == PROCESS_RUNNING)
        processes[previous].state = PROCESS_READY;

    for (int offset = 1; offset <= process_count; offset++) {
        int candidate = (previous + offset) % process_count;
        if (processes[candidate].state != PROCESS_BLOCKED) {
            current_pid = candidate;
            processes[current_pid].state = PROCESS_RUNNING;
            return;
        }
    }

    processes[previous].state = PROCESS_RUNNING;
}

// retorna o processo atualmente em execução
process_t* process_current(void) {
    if (process_count == 0) return 0;
    return &processes[current_pid];
}

/* Salva o frame montado pelo wrapper da IRQ e seleciona o próximo processo.
   Fora de process_run(), o timer permanece apenas como contador de ticks. */
uint32_t process_schedule_from_irq(uint32_t current_esp) {
    if (!scheduler_running || process_count == 0) return current_esp;

    processes[current_pid].esp = current_esp;
    schedule();
    return processes[current_pid].esp;
}

// inicia o primeiro processo remove o entry da stack e pula para ele
void process_run(void) {
    if (process_count == 0) return;

    __asm__ volatile ("cli");
    current_pid = 0;
    processes[0].state = PROCESS_RUNNING;
    scheduler_running = 1;

    // carrega ESP do primeiro processo e salta para ele via iret
    __asm__ volatile (
        "mov %0, %%esp\n"
        "popa\n"
        "iret\n"
        : : "r"(processes[0].esp)
    );
}

/* Cede a CPU pelo mesmo frame de interrupção usado pela preempção. */
void yield(void) {
    __asm__ volatile ("int $0x20" : : : "memory");
}
