#include "graphics.h"
#include "font.h" 
#include "memory.h"
#include "util.h"

extern unsigned int vbe_framebuffer; 

// Наш невидимый буфер (4 байта на пиксель - для скорости расчетов)
unsigned int* backbuffer;

int screen_w = 800;
int screen_h = 600;

int gfx_cursor_x = 0;
int gfx_cursor_y = 0;

void init_graphics() {
    backbuffer = (unsigned int*) kmalloc(screen_w * screen_h * 4); 
    clear_graphics(VGA_BLACK);
}

// ==============================================================
// УНИВЕРСАЛЬНЫЙ ТРАНСЛЯТОР 32-bit BACKBUFFER -> 24-bit VRAM
// ==============================================================
void swap_buffers() {
    // Указатель типа char (1 байт) на реальную видеокарту
    unsigned char* vga = (unsigned char*) vbe_framebuffer;
    
    int ptr = 0; // Смещение в видеопамяти
    
    for (int i = 0; i < screen_w * screen_h; i++) {
        unsigned int color = backbuffer[i]; // Берем 32-битный цвет из нашего буфера
        
        // Пишем строго 3 байта в видеокарту!
        vga[ptr++] = color & 0xFF;         // Синий канал
        vga[ptr++] = (color >> 8) & 0xFF;  // Зеленый канал
        vga[ptr++] = (color >> 16) & 0xFF; // Красный канал
    }
}

// --- ФУНКЦИИ МЫШИ ДЛЯ ТЕРМИНАЛА (РАБОТАЮТ НАПРЯМУЮ С 24-BIT VRAM) ---
void draw_mouse_cli(int x, int y) {
    unsigned char* vga = (unsigned char*) vbe_framebuffer;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            if (x+j >= screen_w || y+i >= screen_h) continue;
            
            int ptr = ((y + i) * screen_w + (x + j)) * 3; // Индекс пикселя * 3 байта
            vga[ptr] = 0xFF;     // B
            vga[ptr+1] = 0xFF;   // G
            vga[ptr+2] = 0xFF;   // R
        }
    }
}

void erase_mouse_cli(int x, int y) {
    unsigned char* vga = (unsigned char*) vbe_framebuffer;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            if (x+j >= screen_w || y+i >= screen_h) continue;
            
            int ptr = ((y + i) * screen_w + (x + j)) * 3;
            // Достаем цвет из 32-битного буфера и раскладываем на 3 байта
            unsigned int color = backbuffer[(y + i) * screen_w + (x + j)];
            vga[ptr] = color & 0xFF;
            vga[ptr+1] = (color >> 8) & 0xFF;
            vga[ptr+2] = (color >> 16) & 0xFF;
        }
    }
}
// ==============================================================

void put_pixel(int x, int y, unsigned int color) {
    if (x >= 0 && x < screen_w && y >= 0 && y < screen_h) {
        backbuffer[(y * screen_w) + x] = color;
    }
}

unsigned int get_pixel(int x, int y) {
    if (x >= 0 && x < screen_w && y >= 0 && y < screen_h) return backbuffer[(y * screen_w) + x];
    return 0;
}

void clear_graphics(unsigned int color) {
    for (int i = 0; i < (screen_w * screen_h); i++) backbuffer[i] = color;
    gfx_cursor_x = 0; gfx_cursor_y = 0;
}

void draw_rectangle(int x, int y, int width, int height, unsigned int color) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) put_pixel(x + j, y + i, color);
    }
}

void draw_char(int x, int y, char c, unsigned int color) {
    if (c < 32 || c > 126) c = '?';
    int font_index = c - 32; 
    for (int cy = 0; cy < 8; cy++) {
        unsigned char row = font8x8[font_index][cy]; 
        for (int cx = 0; cx < 8; cx++) {
            if ((row >> (7 - cx)) & 1) put_pixel(x + cx, y + cy, color);
            else put_pixel(x + cx, y + cy, VGA_BLACK); 
        }
    }
}

void draw_char_transparent(int x, int y, char c, unsigned int color) {
    if (c < 32 || c > 126) c = '?';
    int font_index = c - 32; 
    for (int cy = 0; cy < 8; cy++) {
        unsigned char row = font8x8[font_index][cy]; 
        for (int cx = 0; cx < 8; cx++) {
            if ((row >> (7 - cx)) & 1) put_pixel(x + cx, y + cy, color);
        }
    }
}

void scroll_graphics() {
    if (gfx_cursor_y < screen_h - 10) return; 
    for (int i = 0; i < screen_w * (screen_h - 10); i++) {
        backbuffer[i] = backbuffer[i + screen_w * 10];
    }
    for (int i = screen_w * (screen_h - 10); i < (screen_w * screen_h); i++) {
        backbuffer[i] = VGA_BLACK;
    }
    gfx_cursor_y -= 10;
}

void set_gfx_cursor(int x, int y) { gfx_cursor_x = x; gfx_cursor_y = y; }

void draw_string(char* str, unsigned int color) {
    int i = 0;
    while (str[i] != '\0') {
        scroll_graphics();
        if (str[i] == '\n') {
            gfx_cursor_x = 0; gfx_cursor_y += 10; 
        } else if (str[i] == '\b') {
            if (gfx_cursor_x > 0) {
                gfx_cursor_x -= 8;
                draw_rectangle(gfx_cursor_x, gfx_cursor_y, 8, 10, VGA_BLACK); 
            }
        } else {
            draw_char(gfx_cursor_x, gfx_cursor_y, str[i], color);
            gfx_cursor_x += 8;
            if (gfx_cursor_x >= screen_w) { gfx_cursor_x = 0; gfx_cursor_y += 10; }
        }
        i++;
    }
}