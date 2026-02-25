#include "gui.h"
#include "graphics.h"
#include "mouse.h"
#include "util.h"
#include "fat16.h"

int gui_active = 0;

Window windows[MAX_WINDOWS];
int num_windows = 0;

int dragging_win_id = -1;
int is_dragging = 0;
int drag_off_x = 0;
int drag_off_y = 0;

int prev_mouse_left = 0; 

void init_gui() {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].is_open = 0;
        windows[i].z_index = -1; 
        windows[i].content[0] = '\0';
    }
}

void bring_window_to_front(int win_id) {
    int current_z = windows[win_id].z_index;
    int max_z = -1;
    
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_open && windows[i].z_index > max_z) {
            max_z = windows[i].z_index;
        }
    }
    
    if (current_z == max_z && max_z != -1) return; 
    
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_open && windows[i].z_index > current_z) {
            windows[i].z_index--;
        }
    }
    
    windows[win_id].z_index = max_z; 
}

int create_window(int x, int y, int width, int height, char* title) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!windows[i].is_open) {
            windows[i].id = i;
            windows[i].x = x;
            windows[i].y = y;
            windows[i].width = width;
            windows[i].height = height;
            windows[i].content[0] = '\0';
            
            int t = 0;
            while (title[t] != '\0' && t < 31) {
                windows[i].title[t] = title[t];
                t++;
            }
            windows[i].title[t] = '\0';
            
            windows[i].is_open = 1;

            int max_z = -1;
            for(int j=0; j<MAX_WINDOWS; j++) {
                if(windows[j].is_open && windows[j].z_index > max_z && j != i) {
                    max_z = windows[j].z_index;
                }
            }
            windows[i].z_index = max_z + 1;

            return i; 
        }
    }
    return -1; 
}

void close_window(int win_id) {
    if (win_id >= 0 && win_id < MAX_WINDOWS) {
        windows[win_id].is_open = 0;
        windows[win_id].z_index = -1;
    }
}

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

void render_window(Window* win) {
    if (!win->is_open) return;

    draw_rectangle(win->x + 5, win->y + 5, win->width, win->height, VGA_DGRAY); 
    draw_rectangle(win->x, win->y, win->width, win->height, VGA_LGRAY); 
    draw_rectangle(win->x, win->y, win->width, 1, VGA_WHITE); 
    draw_rectangle(win->x, win->y, 1, win->height, VGA_WHITE); 
    draw_rectangle(win->x, win->y + win->height - 1, win->width, 1, VGA_BLACK); 
    draw_rectangle(win->x + win->width - 1, win->y, 1, win->height, VGA_BLACK); 
    draw_rectangle(win->x + 1, win->y + win->height - 2, win->width - 2, 1, VGA_DGRAY); 
    draw_rectangle(win->x + win->width - 2, win->y + 1, 1, win->height - 2, VGA_DGRAY);

    unsigned int title_color = VGA_BLUE;
    int is_top = 1;
    for(int i = 0; i < MAX_WINDOWS; i++) {
        if(windows[i].is_open && windows[i].z_index > win->z_index) is_top = 0;
    }
    if(!is_top) title_color = VGA_DGRAY;

    draw_rectangle(win->x + 2, win->y + 2, win->width - 4, 14, title_color); 
    for(int i = 0; win->title[i] != '\0'; i++) draw_char_transparent(win->x + 4 + (i * 8), win->y + 5, win->title[i], VGA_WHITE);
    draw_button(win->x + win->width - 14, win->y + 4, 10, 10, "X"); 

    draw_rectangle(win->x + 5, win->y + 20, win->width - 10, win->height - 25, VGA_WHITE); 
    
    // ВНУТРЕННИЙ РЕНДЕР
    if (str_compare(win->title, "File Explorer")) {
        int just_clicked = (mouse_left_pressed && !prev_mouse_left);
        int click_to_send = (is_top && !is_dragging && just_clicked) ? 1 : 0;
        fat16_draw_files(win->x + 10, win->y + 25, VGA_BLACK, mouse_x, mouse_y, click_to_send);
    }
    else {
        // Отрисовка текста программ
        int cx = win->x + 15;
        int cy = win->y + 30;
        for(int i = 0; win->content[i] != '\0' && i < 127; i++) {
            if (win->content[i] == '\n') { cx = win->x + 15; cy += 15; } 
            else { draw_char_transparent(cx, cy, win->content[i], VGA_BLACK); cx += 8; }
        }
    }
}

void gui_start_desktop() {
    init_gui();
    gui_active = 1;
    prev_mouse_left = 0; 
    // ОКОН ПРИ СТАРТЕ НЕТ!
}

void gui_update() {
    int just_clicked = (mouse_left_pressed && !prev_mouse_left);

    if (mouse_left_pressed) {
        if (dragging_win_id == -1) {
            
            if (just_clicked) {
                int top_clicked_win = -1;
                int max_z = -1;

                // Ищем клик по окну
                for (int i = 0; i < MAX_WINDOWS; i++) {
                    if (windows[i].is_open) {
                        if (mouse_x >= windows[i].x && mouse_x <= windows[i].x + windows[i].width &&
                            mouse_y >= windows[i].y && mouse_y <= windows[i].y + windows[i].height) {
                            
                            if (windows[i].z_index > max_z) {
                                max_z = windows[i].z_index;
                                top_clicked_win = i;
                            }
                        }
                    }
                }

                if (top_clicked_win != -1) {
                    Window* win = &windows[top_clicked_win];
                    bring_window_to_front(top_clicked_win);

                    if (mouse_x >= win->x + win->width - 14 && mouse_x <= win->x + win->width - 4 && mouse_y >= win->y + 4 && mouse_y <= win->y + 14) {
                        close_window(top_clicked_win);
                    }
                    else if (mouse_y >= win->y + 2 && mouse_y <= win->y + 16) {
                        is_dragging = 1; dragging_win_id = top_clicked_win;
                        drag_off_x = mouse_x - win->x; drag_off_y = mouse_y - win->y;
                    }
                } 
                else {
                    // ЕСЛИ КЛИКНУЛИ НЕ ПО ОКНУ, проверяем иконки на Рабочем столе:
                    
                    // Клик по иконке HDD (x: 20-50, y: 20-50)
                    if (mouse_x >= 20 && mouse_x <= 50 && mouse_y >= 20 && mouse_y <= 50) {
                        fat16_run("EXPLORE BIN");
                    }
                    // Клик по иконке Trash (x: 20-50, y: 80-110)
                    else if (mouse_x >= 20 && mouse_x <= 50 && mouse_y >= 80 && mouse_y <= 110) {
                        fat16_run("TRASH   BIN");
                    }
                }
            }
        } else {
            windows[dragging_win_id].x = mouse_x - drag_off_x;
            windows[dragging_win_id].y = mouse_y - drag_off_y;
        }
    } else {
        is_dragging = 0; dragging_win_id = -1;
    }

    // Возврат в терминал (клик в правый нижний угол)
    if (just_clicked && mouse_x > 780 && mouse_y > 580) {
        gui_active = 0; 
        return;
    }

    prev_mouse_left = mouse_left_pressed;

    // РЕНДЕР КАДРА
    clear_graphics(VGA_CYAN);           
    draw_icon(20, 20, "HDD");           
    draw_icon(20, 80, "Trash"); 

    for (int curr_z = 0; curr_z < MAX_WINDOWS; curr_z++) {
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (windows[i].is_open && windows[i].z_index == curr_z) render_window(&windows[i]);
        }
    }
}