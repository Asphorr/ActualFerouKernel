#pragma once

#define MAX_WINDOWS 10

typedef struct {
    int id;             // Уникальный номер окна
    int x, y;           // Координаты на экране
    int width, height;  // Размеры
    char title[32];     // Заголовок
    int is_open;        // Открыто ли окно?
    int z_index;        // Слой (0 - самое нижнее, 1 - поверх него и т.д.)
} Window;

extern int gui_active;

void gui_start_desktop();
void gui_update();

// Новое API для окон
int create_window(int x, int y, int width, int height, char* title);
void close_window(int win_id);
void bring_window_to_front(int win_id);