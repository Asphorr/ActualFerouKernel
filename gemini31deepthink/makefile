CC = gcc
LD = ld
ASM = nasm
CFLAGS = -std=gnu17 -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-toplevel-reorder -nostdlib -Wall -Wextra -O3 -mno-sse -mno-sse2 -mno-mmx -mno-avx -mno-avx2 -msoft-float

KERNEL_SRCS = $(filter-out explorer.c sysinfo.c clock.c settings.c libc.c, $(wildcard *.c))
KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)
APPS = explorer.bin sysinfo.bin clock.bin settings.bin

all: app_entry.o app.ld os_image.bin fs.img

# ИСПРАВЛЕНИЕ: Автоматическая генерация Ассемблерной Заглушки для приложений!
app_entry.o:
	echo '[BITS 32]' > app_entry.asm
	echo 'section .text.entry' >> app_entry.asm
	echo 'global _entry' >> app_entry.asm
	echo 'extern _start' >> app_entry.asm
	echo '_entry:' >> app_entry.asm
	echo 'and esp, 0xFFFFFFF0' >> app_entry.asm
	echo 'call _start' >> app_entry.asm
	echo 'mov eax, 25' >> app_entry.asm
	echo 'int 0x80' >> app_entry.asm
	echo '.hang: jmp .hang' >> app_entry.asm
	$(ASM) -f elf32 app_entry.asm -o app_entry.o

# ИСПРАВЛЕНИЕ: Удаляем мусор GCC (/DISCARD/) и ставим app_entry в самое начало!
app.ld:
	echo 'OUTPUT_FORMAT("binary")' > app.ld
	echo 'ENTRY(_entry)' >> app.ld
	echo 'SECTIONS {' >> app.ld
	echo '  . = 0x80000000;' >> app.ld
	echo '  .text : { app_entry.o(.text.entry) *(.text*) }' >> app.ld
	echo '  .rodata : { *(.rodata*) }' >> app.ld
	echo '  .data : { *(.data*) }' >> app.ld
	echo '  .bss : { *(.bss*) }' >> app.ld
	echo '  /DISCARD/ : { *(.note*) *(.eh_frame*) *(.comment*) }' >> app.ld
	echo '}' >> app.ld

os_image.bin: bootloader.bin kernel.bin
	cat bootloader.bin kernel.bin > os_image.bin
	truncate -s 1048576 os_image.bin

bootloader.bin: bootloader.asm
	$(ASM) -f bin bootloader.asm -o bootloader.bin

kernel_entry.o: kernel_entry.asm
	$(ASM) -f elf32 kernel_entry.asm -o kernel_entry.o

$(KERNEL_OBJS): %.o: %.c system.h
	$(CC) $(CFLAGS) -c $< -o $@

kernel.bin: kernel_entry.o $(KERNEL_OBJS)
	$(LD) -m elf_i386 -Ttext 0x10000 --oformat binary kernel_entry.o $(KERNEL_OBJS) -o kernel.bin

%.bin: %.c libc.h app.ld app_entry.o
	$(CC) $(CFLAGS) -c $< -o $*.o
	$(LD) -m elf_i386 -T app.ld app_entry.o $*.o -o $@

fs.img: $(APPS)
	dd if=/dev/zero of=fs.img bs=1M count=64 2>/dev/null
	mkfs.fat -F 16 fs.img >/dev/null
	mcopy -i fs.img explorer.bin ::EXPLORER.BIN
	mcopy -i fs.img sysinfo.bin ::SYSINFO.BIN
	mcopy -i fs.img clock.bin ::CLOCK.BIN
	mcopy -i fs.img settings.bin ::SETTINGS.BIN
	if [ -f WALL.BG ]; then mcopy -i fs.img WALL.BG ::WALL.BG; fi

run: all
	qemu-system-x86_64.exe -m 512 -cpu max -vga std -drive format=raw,file=os_image.bin,index=0,media=disk -drive format=raw,file=fs.img,index=1,media=disk

clean:
	rm -f *.bin *.o *.img *.ld app_entry.asm