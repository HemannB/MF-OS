CC  = gcc
LD  = ld
AS  = nasm

.DEFAULT_GOAL := all

CFLAGS = -m32 -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
         -fno-builtin -fno-stack-protector -nostdlib -nodefaultlibs \
         -MMD -MP -Ikernel

DOOM_CFLAGS = $(CFLAGS) -DDOOM_LIBC_SHIMS -DCMAP256 -DDOOMGENERIC_RESX=320 -DDOOMGENERIC_RESY=200 \
              -Idoomgeneric/doomgeneric \
              -w

ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

DOOM_SRC_DIR = doomgeneric/doomgeneric

# ── Objetos do kernel ────────────────────────────────────────────────
KERNEL_OBJ = \
    boot/boot.o \
    kernel/kernel.o \
    kernel/gdt.o \
    kernel/gdt_flush.o \
    kernel/idt.o \
    kernel/idt_flush.o \
    kernel/pic.o \
    kernel/isr.o \
    kernel/isr_asm.o \
    kernel/timer.o \
    kernel/heap.o \
    kernel/paging.o \
    kernel/process.o \
    kernel/vga13h.o \
    kernel/terminal.o \
    kernel/tests.o \
    kernel/fs.o \
    kernel/libc.o \
    kernel/doom_shims.o

# ── Objetos do doomgeneric (lista exata dos .c presentes) ────────────
DOOM_OBJ = \
    $(DOOM_SRC_DIR)/doomgeneric_mf0s.o \
    $(DOOM_SRC_DIR)/doomgeneric.o \
    $(DOOM_SRC_DIR)/am_map.o \
    $(DOOM_SRC_DIR)/d_event.o \
    $(DOOM_SRC_DIR)/d_items.o \
    $(DOOM_SRC_DIR)/d_iwad.o \
    $(DOOM_SRC_DIR)/d_loop.o \
    $(DOOM_SRC_DIR)/d_main.o \
    $(DOOM_SRC_DIR)/d_mode.o \
    $(DOOM_SRC_DIR)/d_net.o \
    $(DOOM_SRC_DIR)/doomdef.o \
    $(DOOM_SRC_DIR)/doomstat.o \
    $(DOOM_SRC_DIR)/dstrings.o \
    $(DOOM_SRC_DIR)/dummy.o \
    $(DOOM_SRC_DIR)/f_finale.o \
    $(DOOM_SRC_DIR)/f_wipe.o \
    $(DOOM_SRC_DIR)/g_game.o \
    $(DOOM_SRC_DIR)/gusconf.o \
    $(DOOM_SRC_DIR)/hu_lib.o \
    $(DOOM_SRC_DIR)/hu_stuff.o \
    $(DOOM_SRC_DIR)/i_cdmus.o \
    $(DOOM_SRC_DIR)/i_endoom.o \
    $(DOOM_SRC_DIR)/i_input.o \
    $(DOOM_SRC_DIR)/i_joystick.o \
    $(DOOM_SRC_DIR)/i_scale.o \
    $(DOOM_SRC_DIR)/i_sound.o \
    $(DOOM_SRC_DIR)/i_system.o \
    $(DOOM_SRC_DIR)/i_timer.o \
    $(DOOM_SRC_DIR)/i_video.o \
    $(DOOM_SRC_DIR)/icon.o \
    $(DOOM_SRC_DIR)/info.o \
    $(DOOM_SRC_DIR)/m_argv.o \
    $(DOOM_SRC_DIR)/m_bbox.o \
    $(DOOM_SRC_DIR)/m_cheat.o \
    $(DOOM_SRC_DIR)/m_config.o \
    $(DOOM_SRC_DIR)/m_controls.o \
    $(DOOM_SRC_DIR)/m_fixed.o \
    $(DOOM_SRC_DIR)/m_menu.o \
    $(DOOM_SRC_DIR)/m_misc.o \
    $(DOOM_SRC_DIR)/m_random.o \
    $(DOOM_SRC_DIR)/memio.o \
    $(DOOM_SRC_DIR)/mus2mid.o \
    $(DOOM_SRC_DIR)/p_ceilng.o \
    $(DOOM_SRC_DIR)/p_doors.o \
    $(DOOM_SRC_DIR)/p_enemy.o \
    $(DOOM_SRC_DIR)/p_floor.o \
    $(DOOM_SRC_DIR)/p_inter.o \
    $(DOOM_SRC_DIR)/p_lights.o \
    $(DOOM_SRC_DIR)/p_map.o \
    $(DOOM_SRC_DIR)/p_maputl.o \
    $(DOOM_SRC_DIR)/p_mobj.o \
    $(DOOM_SRC_DIR)/p_plats.o \
    $(DOOM_SRC_DIR)/p_pspr.o \
    $(DOOM_SRC_DIR)/p_saveg.o \
    $(DOOM_SRC_DIR)/p_setup.o \
    $(DOOM_SRC_DIR)/p_sight.o \
    $(DOOM_SRC_DIR)/p_spec.o \
    $(DOOM_SRC_DIR)/p_switch.o \
    $(DOOM_SRC_DIR)/p_telept.o \
    $(DOOM_SRC_DIR)/p_tick.o \
    $(DOOM_SRC_DIR)/p_user.o \
    $(DOOM_SRC_DIR)/r_bsp.o \
    $(DOOM_SRC_DIR)/r_data.o \
    $(DOOM_SRC_DIR)/r_draw.o \
    $(DOOM_SRC_DIR)/r_main.o \
    $(DOOM_SRC_DIR)/r_plane.o \
    $(DOOM_SRC_DIR)/r_segs.o \
    $(DOOM_SRC_DIR)/r_sky.o \
    $(DOOM_SRC_DIR)/r_things.o \
    $(DOOM_SRC_DIR)/s_sound.o \
    $(DOOM_SRC_DIR)/sha1.o \
    $(DOOM_SRC_DIR)/sounds.o \
    $(DOOM_SRC_DIR)/st_lib.o \
    $(DOOM_SRC_DIR)/st_stuff.o \
    $(DOOM_SRC_DIR)/statdump.o \
    $(DOOM_SRC_DIR)/tables.o \
    $(DOOM_SRC_DIR)/v_video.o \
    $(DOOM_SRC_DIR)/w_checksum.o \
    $(DOOM_SRC_DIR)/w_file.o \
    $(DOOM_SRC_DIR)/w_file_stdc.o \
    $(DOOM_SRC_DIR)/w_main.o \
    $(DOOM_SRC_DIR)/w_wad.o \
    $(DOOM_SRC_DIR)/wi_stuff.o \
    $(DOOM_SRC_DIR)/z_zone.o

OBJ    = $(KERNEL_OBJ) $(DOOM_OBJ)
KERNEL = mf0s.kernel
ISO    = mf0s.iso
WAD    = iso/boot/doom1.wad
DEPS   = $(OBJ:.o=.d)

-include $(DEPS)

all: $(ISO)

# ── Regras de compilação ─────────────────────────────────────────────

boot/boot.o: boot/boot.asm
	$(AS) $(ASFLAGS) $< -o $@

kernel/gdt_flush.o: kernel/gdt_flush.asm
	$(AS) $(ASFLAGS) $< -o $@

kernel/idt_flush.o: kernel/idt_flush.asm
	$(AS) $(ASFLAGS) $< -o $@

kernel/isr_asm.o: kernel/isr_asm.asm
	$(AS) $(ASFLAGS) $< -o $@

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DOOM_SRC_DIR)/%.o: $(DOOM_SRC_DIR)/%.c
	$(CC) $(DOOM_CFLAGS) -c $< -o $@

# ── Link ─────────────────────────────────────────────────────────────
$(KERNEL): $(OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

# ── ISO ──────────────────────────────────────────────────────────────
$(WAD):
	@test -f $(WAD) || { echo "error: place a Doom WAD at $(WAD)"; false; }

$(ISO): $(KERNEL) $(WAD) iso/boot/grub/grub.cfg
	cp $(KERNEL) iso/boot/mf0s.kernel
	grub-mkrescue -o $(ISO) iso/

check: $(KERNEL)
	grub-file --is-x86-multiboot $(KERNEL)

smoke: $(ISO)
	@log=$$(mktemp /tmp/mf0s-smoke.XXXXXX); \
	status=0; \
	timeout 10s qemu-system-i386 -vga std -cdrom $(ISO) -m 512M \
	    -display none -monitor none -serial none -no-reboot -no-shutdown \
	    -debugcon file:$$log || status=$$?; \
	if [ $$status -ne 124 ] || ! grep -q "MF-0S> " $$log; then \
	    echo "smoke test failed; debug log: $$log"; \
	    cat $$log; \
	    exit 1; \
	fi; \
	rm -f $$log; \
	echo "smoke test passed: shell prompt reached"

# ── QEMU ─────────────────────────────────────────────────────────────
run: $(ISO)
	qemu-system-i386 -vga std -cdrom $(ISO) -m 512M \
	    -device isa-debug-exit,iobase=0xf4,iosize=0x04 || true

clean:
	rm -f $(KERNEL_OBJ) $(DOOM_OBJ) $(DEPS) $(KERNEL) $(ISO) iso/boot/mf0s.kernel

.PHONY: all check smoke run clean
