#include "gui.h"
#include "graphics.h"
#include "mouse.h"
#include "util.h"
#include "fat16.h"

int gui_active = 0;
int win_x = 200; // Центрируем стартовую позицию для 800x600
int win_y = 150;
int win_w = 400; // Увеличили окно!
int win_h = 250;
int win_open = 1;

int is_dragging = 0;
int drag_off_x = 0;
int drag_off_y = 0;

// Отрисовка маленькой иконки папки на рабочем столе
void draw_icon(int x, int y, char* label) {
    draw_rectangle(x, y, 16, 12, VGA_YELLOW);        
    draw_rectangle(x, y-2, 6, 2, VGA_YELLOW);        
    draw_rectangle(x+1, y+1, 14, 10, VGA_BROWN);     
    
    int text_x = x - ((str_length(label) * 8) / 2) + 8; 
    for(int i = 0; label[i] != '\0'; i++) draw_char_transparent(text_x + (i * 8), y + 15, label[i], VGA_WHITE); 
}

void draw_button(int x, int y, int width, int height, char* text) {
    draw_rectangle(x, y, width, height, VGA_LGRAY); 
    draw_rectangle(x, y, width, 1, VGA_WHITE);      
    draw_rectangle(x, y, 1, height, VGA_WHITE);     
    draw_rectangle(x, y + height - 1, width, 1, VGA_DGRAY); 
    draw_rectangle(x + width - 1, y, 1, height, VGA_DGRAY); 
    
    int text_x = x + (width / 2) - 4; 
    int text_y = y + (height / 2) - 4;
    for(int i = 0; text[i] != '\0'; i++) draw_char_transparent(text_x + (i * 8), text_y, text[i], VGA_BLACK); 
}

void draw_window(int x, int y, int width, int height, char* title) {
    if (!win_open) return; 

    draw_rectangle(x + 5, y + 5, width, height, VGA_DGRAY); // Тень

    draw_rectangle(x, y, width, height, VGA_LGRAY); 
    draw_rectangle(x, y, width, 1, VGA_WHITE); 
    draw_rectangle(x, y, 1, height, VGA_WHITE); 
    draw_rectangle(x, y + height - 1, width, 1, VGA_BLACK); 
    draw_rectangle(x + width - 1, y, 1, height, VGA_BLACK); 
    draw_rectangle(x + 1, y + height - 2, width - 2, 1, VGA_DGRAY); 
    draw_rectangle(x + width - 2, y + 1, 1, height - 2, VGA_DGRAY);

    draw_rectangle(x + 2, y + 2, width - 4, 14, VGA_BLUE); // Шапка
    for(int i = 0; title[i] != '\0'; i++) draw_char_transparent(x + 4 + (i * 8), y + 5, title[i], VGA_WHITE);
    draw_button(x + width - 14, y + 4, 10, 10, "X"); 

    // Внутренняя область окна (Белый фон папки)
    draw_rectangle(x + 5, y + 20, width - 10, height - 25, VGA_WHITE); 
    
    // Передаем координаты и статус клика мыши в файловую систему
    int click = (!is_dragging) ? mouse_left_pressed : 0;
    fat16_draw_files(x + 10, y + 25, VGA_BLACK, mouse_x, mouse_y, click);
}

void gui_start_desktop() {
    gui_active = 1;
    win_open = 1;
}

void gui_update() {
    if (win_open) {
        if (mouse_left_pressed) {
            if (!is_dragging) {
                // Клик на [X] (Используем win_w вместо 160)
                if (mouse_x >= win_x + win_w - 14 && mouse_x <= win_x + win_w - 4 && mouse_y >= win_y + 4 && mouse_y <= win_y + 14) {
                    win_open = 0; 
                }
                // Клик на шапку
                else if (mouse_x >= win_x + 2 && mouse_x <= win_x + win_w - 4 && mouse_y >= win_y + 2 && mouse_y <= win_y + 16) {
                    is_dragging = 1; 
                    drag_off_x = mouse_x - win_x; 
                    drag_off_y = mouse_y - win_y;
                }
            } else {
                win_x = mouse_x - drag_off_x; 
                win_y = mouse_y - drag_off_y; 
            }
        } else {
            is_dragging = 0;
        }
    }

    // Секретная кнопка выхода (Клик в нижний правый угол ЭКРАНА 800x600)
    if (mouse_left_pressed && mouse_x > 780 && mouse_y > 580) {
        gui_active = 0; 
        return;
    }

    clear_graphics(VGA_CYAN);           
    draw_icon(20, 20, "HDD");           
    draw_icon(20, 80, "Trash"); // Раздвинули иконки
    
    draw_window(win_x, win_y, win_w, win_h, "File Explorer"); 
}