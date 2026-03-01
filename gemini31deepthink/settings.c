#include "libc.h"
static char files[512] = {0}; static int loaded = 0;
void _start() { 
    loaded = 0; for(int i = 0; i < 512; i++) files[i] = 0; 
    int win = create_window("Settings", 300, 200); 
    int prev_click = 0;
    while (is_window_open(win)) {
        draw_rect(win, 0, 0, 300, 200, 0x1E1E1E); draw_text(win, 10, 10, "Personalization", 0x00FFFF); draw_rect(win, 10, 35, 280, 1, 0x555555); draw_text(win, 10, 45, "Wallpapers:", 0xAAAAAA);
        if (!loaded) { get_files(files); loaded = 1; }
        int mx, my, click; get_mouse(win, &mx, &my, &click); 
        int y = 75, i = 0;
        while (files[i]) {
            char name[32]; int n = 0; while (files[i] && files[i] != '\n') { if(n < 31) name[n++] = files[i]; i++; }
            name[n] = '\0'; if (files[i] == '\n') i++;
            if (n >= 11 && name[8] == 'B' && name[9] == 'G') {
                int hover = (mx > 10 && mx < 290 && my > y - 5 && my < y + 20);
                draw_rect(win, 10, y - 5, 280, 24, hover ? (click ? 0x0055AA : 0x3E3E42) : 0x2D2D30); 
                char dname[32]; int di = 0; 
                for (int k = 0; k < 8; k++) { if (name[k] != ' ') dname[di++] = name[k]; } dname[di++] = '.'; 
                for (int k = 8; k < 11; k++) { if (name[k] != ' ') dname[di++] = name[k]; } dname[di] = '\0';
                draw_text(win, 20, y, dname, 0xFFFFFF);
                if (hover && click && !prev_click) set_wallpaper(name);
                y += 30;
            }
        }
        prev_click = click; 
        update_window(win); sleep_ms(16);
    }
}