bits 32
global context_switch

; void context_switch(registers_t *old, registers_t *new)
context_switch:
    mov eax, [esp + 4]   ; carrega o primeiro argumento: ponteiro para old
    mov ecx, [esp + 8]   ; carrega o segundo argumento: ponteiro para new

    ; salva o estado do processo atual em old
    mov [eax + 0],  eax  ; salva o valor atual de EAX no campo old->eax
    mov [eax + 4],  ebx  ; salva EBX no campo old->ebx
    mov [eax + 8],  ecx  ; salva ECX no campo old->ecx
    mov [eax + 12], edx  ; salva EDX no campo old->edx
    mov [eax + 16], esp  ; salva ESP no campo old->esp
    mov [eax + 20], ebp  ; salva EBP no campo old->ebp
    mov [eax + 24], esi  ; salva ESI no campo old->esi
    mov [eax + 28], edi  ; salva EDI no campo old->edi

    ; restaura o estado do novo processo a partir de new
    mov ebx, [ecx + 4]   ; carrega EBX do campo new->ebx
    mov edx, [ecx + 12]  ; carrega EDX do campo new->edx
    mov esp, [ecx + 16]  ; carrega ESP do campo new->esp
    mov ebp, [ecx + 20]  ; carrega EBP do campo new->ebp
    mov esi, [ecx + 24]  ; carrega ESI do campo new->esi
    mov edi, [ecx + 28]  ; carrega EDI do campo new->edi

    ; salta para o endereço de instrução salvo no novo processo
    jmp [ecx + 32]       ; pula para new->eip

section .note.GNU-stack noalloc noexec nowrite progbits