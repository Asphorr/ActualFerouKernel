#include "fat16.h"
#include "disk.h"
#include "memory.h"
#include "util.h"
#include "graphics.h"
#include "mouse.h"

#define PARTITION_START 2048 

unsigned int root_dir_start;
unsigned int data_start;
unsigned char sectors_per_cluster;

// НОВОЕ: Единый глобальный буфер для чтения секторов.
// Он занимает 512 байт всегда и не вызывает утечек памяти при перерисовке GUI!
unsigned char fat16_buffer[512];

void fat16_init() {
    read_sector(PARTITION_START, fat16_buffer);

    unsigned short reserved_sectors = *(unsigned short*)(fat16_buffer + 0x0E);
    unsigned char num_fats = *(unsigned char*)(fat16_buffer + 0x10);
    unsigned short sectors_per_fat = *(unsigned short*)(fat16_buffer + 0x16);
    unsigned short root_dir_entries = *(unsigned short*)(fat16_buffer + 0x11);
    sectors_per_cluster = *(unsigned char*)(fat16_buffer + 0x0D);

    unsigned int fat_start = PARTITION_START + reserved_sectors;
    root_dir_start = fat_start + (num_fats * sectors_per_fat);
    
    unsigned int root_dir_sectors = (root_dir_entries * 32) / 512;
    data_start = root_dir_start + root_dir_sectors;
}

void fat16_ls() {
    read_sector(root_dir_start, fat16_buffer); 

    draw_string("Files on DISK (FAT16):\n", VGA_YELLOW);

    for (int i = 0; i < 512; i += 32) {
        if (fat16_buffer[i] == 0x00) break; 
        if (fat16_buffer[i] == 0xE5) continue; 
        if (fat16_buffer[i + 0x0B] == 0x0F) continue; 
        if (fat16_buffer[i + 0x0B] & 0x08) continue; 
        if (fat16_buffer[i + 0x0B] & 0x10) continue; 

        char filename[12];
        memory_copy((char*)&fat16_buffer[i], filename, 11);
        filename[11] = '\0';

        unsigned int file_size = *(unsigned int*)&fat16_buffer[i + 0x1C];
        char size_str[16];
        int_to_string(file_size, size_str);

        draw_string(" - ", VGA_WHITE);
        draw_string(filename, VGA_LCYAN);
        draw_string(" (Size: ", VGA_DGRAY);
        draw_string(size_str, VGA_DGRAY);
        draw_string(" bytes)\n", VGA_DGRAY);
    }
}

void fat16_cat_test() {
    read_sector(root_dir_start, fat16_buffer);

    for (int i = 0; i < 512; i += 32) {
        if (fat16_buffer[i] == 0x00) break;

        char filename[12];
        memory_copy((char*)&fat16_buffer[i], filename, 11);
        filename[11] = '\0';

        if (str_compare(filename, "TEST    TXT")) {
            unsigned short starting_cluster = *(unsigned short*)&fat16_buffer[i + 0x1A];
            unsigned int file_size = *(unsigned int*)&fat16_buffer[i + 0x1C];
            unsigned int cluster_lba = data_start + (starting_cluster - 2) * sectors_per_cluster;

            // Читаем текст прямо в глобальный буфер
            read_sector(cluster_lba, fat16_buffer);
            fat16_buffer[file_size] = '\0'; 

            draw_string("Contents of TEST.TXT:\n", VGA_YELLOW);
            draw_string((char*)fat16_buffer, VGA_WHITE);
            draw_string("\n", VGA_WHITE);
            return;
        }
    }
    draw_string("File TEST.TXT not found!\n", VGA_LRED);
}

void fat16_run(char* target_filename) {
    read_sector(root_dir_start, fat16_buffer);

    for (int i = 0; i < 512; i += 32) {
        if (fat16_buffer[i] == 0x00) break;

        char filename[12];
        memory_copy((char*)&fat16_buffer[i], filename, 11);
        filename[11] = '\0';

        if (str_compare(filename, target_filename)) {
            unsigned short starting_cluster = *(unsigned short*)&fat16_buffer[i + 0x1A];
            unsigned int cluster_lba = data_start + (starting_cluster - 2) * sectors_per_cluster;

            unsigned char* app_buffer = (unsigned char*) kmalloc(512); 
            read_sector(cluster_lba, app_buffer);

                        void (*external_program)() = (void*) app_buffer;
            mouse_left_pressed = 0;
            
            // МАГИЯ: Мы не запускаем код. Мы просто отдаем его Планировщику!
            extern void create_task(void (*entry_point)());
            create_task(external_program);
            
            // Ядро моментально возвращается к отрисовке GUI! 
            // А таймер сам начнет выполнять программу в фоне 100 раз в секунду.
            return;
        }
    }
}

void fat16_draw_files(int start_x, int start_y, unsigned char text_color, int m_x, int m_y, int m_click) {
    read_sector(root_dir_start, fat16_buffer); 

    int current_y = start_y;

    for (int i = 0; i < 512; i += 32) {
        if (fat16_buffer[i] == 0x00) break; 
        if (fat16_buffer[i] == 0xE5) continue; 
        if (fat16_buffer[i + 0x0B] == 0x0F) continue; 
        if (fat16_buffer[i + 0x0B] & 0x08) continue; 
        if (fat16_buffer[i + 0x0B] & 0x10) continue; 

        char filename[12];
        memory_copy((char*)&fat16_buffer[i], filename, 11);
        filename[11] = '\0';

        // Проверка клика по файлу
        if (m_click && m_x >= start_x && m_x <= start_x + 100 && m_y >= current_y && m_y <= current_y + 10) {
            if (filename[8] == 'B' && filename[9] == 'I' && filename[10] == 'N') {
                fat16_run(filename); 
                return; 
            }
        }

        // Рисуем иконку (Листок бумаги)
        draw_rectangle(start_x, current_y, 8, 10, VGA_WHITE);       
        draw_rectangle(start_x + 6, current_y, 2, 2, VGA_LGRAY);    

        int cx = start_x + 12;
        for (int j = 0; filename[j] != '\0'; j++) {
            draw_char_transparent(cx + (j * 8), current_y + 1, filename[j], text_color);
        }

        current_y += 14;
    }
}