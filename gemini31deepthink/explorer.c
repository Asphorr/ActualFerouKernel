#include "libc.h"
static char files[512] = {0}; static int loaded = 0;
void _start() { 
    loaded = 0; for (int i = 0; i < 512; i++) files[i] = 0; 
    int win = create_window("File Explorer", 300, 240); 
    int prev_click = 0; // ИСПРАВЛЕНИЕ: Переменная хранится в защищенной памяти программы!
    while (is_window_open(win)) {
        draw_rect(win, 0, 0, 300, 240, 0x1E1E1E); draw_text(win, 10, 10, "Drive D: (FAT16)", 0x00FFFF); draw_rect(win, 10, 35, 280, 1, 0x555555);
        if (!loaded) { get_files(files); loaded = 1; } 
        int mx, my, click; get_mouse(win, &mx, &my, &click); 
        int y = 45, i = 0;
        while (files[i]) {
            char name[32]; int n = 0; while (files[i] && files[i] != '\n') { if (n < 31) name[n++] = files[i]; i++; }
            name[n] = '\0'; if (files[i] == '\n') i++;
            int hover = (mx > 10 && mx < 290 && my > y - 2 && my < y + 18);
            if (hover) draw_rect(win, 10, y - 2, 280, 20, 0x3E3E42); 
            draw_text(win, 15, y, name, hover ? 0xFFFFFF : 0xAAAAAA);
            if (hover && click && !prev_click) exec(name);
            y += 25;
        }
        prev_click = click; 
        update_window(win); sleep_ms(16); 
    }
}