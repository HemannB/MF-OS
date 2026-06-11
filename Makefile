CC  = gcc # compilador C 
LD  = ld # linker
AS  = nasm # montador

# Flags de compilação
CFLAGS  = -m32 -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
          -fno-builtin -fno-stack-protector -nostdlib -nodefaultlibs 
# m32: gera código de 32-bit
# std=gnu99: padrão C99 com extensões GNU necessário para o  Assembly
# O2: otimização nível 2
# Wall -Wextra: ativa todos os warnings
# fno-builtin: não substituir funções como memcpy por versões internas do GCC
# fno-stack-protector: desativa o stack canary
# nodefaultlibs: não linka bibliotecas padrão automaticamente

ASFLAGS = -f elf32 # diz ao NASM para gerar um arquivo objeto no formato ELF de 32-bit
LDFLAGS = -m elf_i386 -T linker.ld # diz ao linker para gerar um executável ELF de 32-bit e -T linker.ld usa o linker script

# lista de arquivos objeto que compõem o kernel
OBJ = boot/boot.o kernel/kernel.o kernel/gdt.o kernel/gdt_flush.o kernel/idt.o kernel/idt_flush.o kernel/pic.o kernel/isr.o kernel/isr_asm.o kernel/timer.o kernel/heap.o kernel/paging.o kernel/process.o kernel/switch.o kernel/vga13h.o kernel/terminal.o kernel/tests.o kernel/fs.o
KERNEL = mf0s.kernel # nome do arquivo do kernel
ISO    = mf0s.iso # nome do arquivo ISO final

all: $(ISO) # alvo padrão para construir a ISO

# Regras de compilação
%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para linkar o kernel
$(KERNEL): $(OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

# Regra para criar a imagem ISO
$(ISO): $(KERNEL)
	cp $(KERNEL) iso/boot/mf0s.kernel
	grub-mkrescue -o $(ISO) iso/

# Regra para rodar a ISO no QEMU
run: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -device isa-debug-exit,iobase=0xf4,iosize=0x04 || true

clean:
	rm -f $(OBJ) $(KERNEL) $(ISO) iso/boot/mf0s.kernel

.PHONY: all run clean