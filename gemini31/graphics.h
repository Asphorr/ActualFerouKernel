#pragma once

// TRUE COLOR (32-bit RGB Palette)
#define VGA_BLACK   0x000000
#define VGA_BLUE    0x0000AA
#define VGA_GREEN   0x00AA00
#define VGA_CYAN    0x00AAAA
#define VGA_RED     0xAA0000
#define VGA_MAGENTA 0xAA00AA
#define VGA_BROWN   0xAA5500
#define VGA_LGRAY   0xAAAAAA
#define VGA_DGRAY   0x555555
#define VGA_LBLUE   0x5555FF
#define VGA_LGREEN  0x55FF55
#define VGA_LCYAN   0x55FFFF
#define VGA_LRED    0xFF5555
#define VGA_LMAGENTA 0xFF55FF
#define VGA_YELLOW  0xFFFF55
#define VGA_WHITE   0xFFFFFF

void init_graphics();   
void swap_buffers();    

// Внимание: все цвета теперь unsigned int!
void put_pixel(int x, int y, unsigned int color);
unsigned int get_pixel(int x, int y);
void clear_graphics(unsigned int color);
void draw_rectangle(int x, int y, int width, int height, unsigned int color);

void draw_char(int x, int y, char c, unsigned int color);
void draw_char_transparent(int x, int y, char c, unsigned int color);
void draw_string(char* str, unsigned int color);

void set_gfx_cursor(int x, int y);
void scroll_graphics();

void draw_mouse_cli(int x, int y);
void erase_mouse_cli(int x, int y);