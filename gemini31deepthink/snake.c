#include "libc.h"

#define GRID 20
#define TILE 12

// ИСПРАВЛЕНИЕ: Массивы теперь в глобальной памяти (.bss), что 100% спасает SSE!
int snake_x[400];
int snake_y[400];

void _start() {
    int win = create_window("Snake Game", GRID * TILE, GRID * TILE);
    
    int length = 4, score = 0;
    int dir_x = 1, dir_y = 0;
    int apple_x = 15, apple_y = 10;
    int game_over = 0;

    for (int i = 0; i < length; i++) { snake_x[i] = 5 - i; snake_y[i] = 5; }

    while (is_window_open(win)) {
        char k = get_key(win);
        
        if (!game_over) {
            if (k == 'w' && dir_y == 0) { dir_x = 0; dir_y = -1; }
            if (k == 's' && dir_y == 0) { dir_x = 0; dir_y = 1; }
            if (k == 'a' && dir_x == 0) { dir_x = -1; dir_y = 0; }
            if (k == 'd' && dir_x == 0) { dir_x = 1; dir_y = 0; }

            for (int i = length - 1; i > 0; i--) {
                snake_x[i] = snake_x[i-1]; snake_y[i] = snake_y[i-1];
            }
            snake_x[0] += dir_x; snake_y[0] += dir_y;

            if (snake_x[0] < 0 || snake_x[0] >= GRID || snake_y[0] < 0 || snake_y[0] >= GRID) {
                game_over = 1;
            }
            
            for (int i = 1; i < length; i++) {
                if (snake_x[0] == snake_x[i] && snake_y[0] == snake_y[i]) { game_over = 1; }
            }

            if (snake_x[0] == apple_x && snake_y[0] == apple_y) {
                if (length < 399) {
                    snake_x[length] = snake_x[length-1];
                    snake_y[length] = snake_y[length-1];
                    length++; score += 10;
                }
                apple_x = rand() % GRID; apple_y = rand() % GRID;
            }
        } else {
            if (k == ' ') {
                game_over = 0; length = 4; score = 0; dir_x = 1; dir_y = 0;
                for (int i = 0; i < length; i++) { snake_x[i] = 5 - i; snake_y[i] = 5; }
            }
        }

        draw_rect(win, 0, 0, GRID * TILE, GRID * TILE, 0x1E1E1E); 
        
        if (game_over) {
            draw_text(win, 50, 100, "GAME OVER!", 0xFF5555);
            draw_text(win, 30, 120, "Press SPACE to retry", 0xAAAAAA);
        } else {
            draw_rect(win, apple_x * TILE, apple_y * TILE, TILE, TILE, 0xFF3333); 
            for (int i = 0; i < length; i++) { 
                draw_rect(win, snake_x[i] * TILE + 1, snake_y[i] * TILE + 1, TILE - 2, TILE - 2, i == 0 ? 0x00FF00 : 0x00AA00);
            }
        }

        char s_buf[16]; to_string(score, s_buf);
        draw_text(win, 5, 5, "Score:", 0xFFFFFF); draw_text(win, 60, 5, s_buf, 0xFFFFFF);

        update_window(win);
        sleep_ms(80); 
    }
    
    exit_app(); // Сообщаем Ядру, что процесс чисто завершился!
}