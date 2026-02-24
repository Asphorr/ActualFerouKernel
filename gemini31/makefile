# Папка для всех скомпилированных файлов
BUILD_DIR = build

# Исходники
C_SOURCES = $(wildcard *.c)
# Меняем пути для объектных файлов (теперь они будут в build/*.o)
OBJ = $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SOURCES))

all: run

# Создаем папку build, если ее нет
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# --- Ассемблерные файлы ---
$(BUILD_DIR)/bootloader.bin: bootloader.asm | $(BUILD_DIR)
	nasm -f bin bootloader.asm -o $@

$(BUILD_DIR)/kernel_entry.o: kernel_entry.asm | $(BUILD_DIR)
	nasm -f elf32 kernel_entry.asm -o $@

$(BUILD_DIR)/app.bin: app.asm | $(BUILD_DIR)
	nasm -f bin app.asm -o $@

# --- Компиляция Си-файлов ---
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	gcc -m32 -ffreestanding -fno-pie -fno-pic -fno-stack-protector -c $< -o $@

# --- Линковка Ядра ---
$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel_entry.o $(OBJ)
	ld -m elf_i386 -o $@ -Ttext 0x1000 $^ --oformat binary

# ==========================================
# СБОРКА ОБРАЗА ДИСКА (FAT16 HDD 32MB)
# ==========================================
$(BUILD_DIR)/hdd.img: $(BUILD_DIR)/bootloader.bin $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/app.bin
	dd if=/dev/zero of=$@ bs=512 count=65536
	
	parted -s $@ mklabel msdos
	parted -s $@ mkpart primary fat16 1MiB 100%
	
	dd if=$(BUILD_DIR)/bootloader.bin of=$@ conv=notrunc
	dd if=$(BUILD_DIR)/kernel.bin of=$@ seek=1 conv=notrunc
	
	mformat -i $@@@1048576 -v MY_OS_DISK ::
	
	echo "Hello from FAT16! This is a real text file." > test.txt
	mcopy -i $@@@1048576 test.txt ::/TEST.TXT
	rm test.txt
	
	# Кладем нашу программу на диск
	mcopy -i $@@@1048576 $(BUILD_DIR)/app.bin ::/APP.BIN

run: $(BUILD_DIR)/hdd.img
	qemu-system-x86_64.exe -vga std -global VGA.vgamem_mb=16 -drive format=raw,file=$(BUILD_DIR)/hdd.img

clean:
	rm -rf $(BUILD_DIR)