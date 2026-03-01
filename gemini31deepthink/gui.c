#include "system.h"

#define MAX_WINS 20 

// ИСПРАВЛЕНИЕ: Добавлен owner_pid для защиты от зомби-окон
typedef struct { int active, minimized, x, y, w, h; char title[32]; uint32_t* front_canvas; uint32_t* back_canvas; int state[16]; int owner_pid; } Window;
Window wins[MAX_WINS]; int z_order[MAX_WINS]; int win_count = 0;
int drag_win = -1, drag_off_x = 0, drag_off_y = 0, prev_click = 0; static int spawn_offset = 0;
int focus_changed_this_click = 0; 
int start_menu_open = 0; char installed_apps[512] = {0}; int apps_loaded = 0;

volatile int gui_needs_redraw = 1;

int gui_is_focused(int id) { if (win_count > 0 && z_order[win_count-1] == id && !wins[id].minimized) return 1; return 0; }
int gui_is_window_open(int id) { if (id < 0 || id >= MAX_WINS) return 0; return wins[id].active; }

// ИСПРАВЛЕНИЕ: Эта функция навсегда удаляет окна мертвого приложения с экрана!
void gui_kill_windows(int pid) {
    for (int i = 0; i < MAX_WINS; i++) {
        if (wins[i].active && wins[i].owner_pid == pid) {
            wins[i].active = 0;
            if (wins[i].front_canvas) kfree(wins[i].front_canvas);
            if (wins[i].back_canvas) kfree(wins[i].back_canvas);
            
            // Чиним Z-порядок
            int idx = -1;
            for (int j = 0; j < win_count; j++) { if (z_order[j] == i) { idx = j; break; } }
            if (idx != -1) {
                for (int j = idx; j < win_count - 1; j++) { z_order[j] = z_order[j+1]; }
                win_count--;
            }
        }
    }
    gui_needs_redraw = 1;
    mark_dirty(0, 0, screen_w, screen_h);
}

int gui_register_window(const char* title, int w, int h) {
    extern volatile int current_task;
    for (int i = 0; i < MAX_WINS; i++) {
        if (!wins[i].active) {
            wins[i].active = 1; wins[i].minimized = 0; wins[i].owner_pid = current_task;
            wins[i].x = 100 + spawn_offset; wins[i].y = 80 + spawn_offset; spawn_offset = (spawn_offset + 30) % 300; 
            wins[i].w = w; wins[i].h = h;
            int j = 0; while (title[j] && j < 31) { wins[i].title[j] = title[j]; j++; } wins[i].title[j] = '\0';
            wins[i].front_canvas = kmalloc(w * h * 4); wins[i].back_canvas = kmalloc(w * h * 4); 
            fast_fill32(wins[i].front_canvas, 0, w * h); fast_fill32(wins[i].back_canvas, 0, w * h);
            for (int s = 0; s < 16; s++) { wins[i].state[s] = 0; }
            z_order[win_count++] = i; 
            gui_needs_redraw = 1; mark_dirty(0, 0, screen_w, screen_h);
            return i;
        }
    } return -1;
}

void gui_swap_window_buffers(int id) {
    if (id < 0 || id >= MAX_WINS || !wins[id].active) return;
    uint32_t flags; __asm__ volatile("pushf\n\tpop %0\n\tcli" : "=r"(flags)); 
    uint32_t* temp = wins[id].front_canvas; wins[id].front_canvas = wins[id].back_canvas; wins[id].back_canvas = temp;
    if (flags & 0x200) __asm__ volatile("sti");
    
    if (!wins[id].minimized) {
        gui_needs_redraw = 1;
        mark_dirty(wins[id].x - 10, wins[id].y - 10, wins[id].w + 20, wins[id].h + 40); 
    }
}

void gui_draw_rect_win(int id, int x, int y, int w, int h, uint32_t c) {
    if (id < 0 || id >= MAX_WINS || !wins[id].active) return;
    int W = wins[id].w, H = wins[id].h; uint32_t* cv = wins[id].back_canvas; 
    int sx = (x < 0) ? 0 : x, sy = (y < 0) ? 0 : y, ex = (x + w > W) ? W : x + w, ey = (y + h > H) ? H : y + h;
    if (sx >= ex || sy >= ey || sx < 0 || sy < 0) return; 
    for (int i = sy; i < ey; i++) fast_fill32(&cv[i * W + sx], c, ex - sx); 
}

void gui_draw_alpha_win(int id, int x, int y, int w, int h, uint32_t arg) {
    if (id < 0 || id >= MAX_WINS || !wins[id].active) return;
    int W = wins[id].w, H = wins[id].h; uint32_t* cv = wins[id].back_canvas; 
    uint32_t color = arg & 0xFFFFFF; uint8_t alpha = (arg >> 24) & 0xFF;
    int sx = (x < 0) ? 0 : x, sy = (y < 0) ? 0 : y, ex = (x + w > W) ? W : x + w, ey = (y + h > H) ? H : y + h;
    if (sx >= ex || sy >= ey || sx < 0 || sy < 0) return; 
    uint32_t sr = (color >> 16) & 0xFF, sg = (color >> 8) & 0xFF, sb = color & 0xFF;
    for (int i = sy; i < ey; i++) { 
        uint32_t* row = &cv[i * W]; 
        for (int j = sx; j < ex; j++) {
            uint32_t bg = row[j]; uint32_t br = (bg>>16)&0xFF, bg_g = (bg>>8)&0xFF, bb = bg&0xFF;
            row[j] = (((sr * alpha + br * (255 - alpha)) >> 8) << 16) | (((sg * alpha + bg_g * (255 - alpha)) >> 8) << 8) | ((sb * alpha + bb * (255 - alpha)) >> 8);
        }
    }
}

void gui_draw_text_win(int id, int x, int y, const char* str, uint32_t c) {
    if (id < 0 || id >= MAX_WINS || !wins[id].active) return;
    extern const uint8_t font8x16[96][16]; int cx = x;
    for (int i = 0; str[i]; i++) {
        if (str[i] == '\n') { y += 16; cx = x; continue; }
        if (str[i] >= 32 && str[i] <= 126) {
            const uint8_t* g = font8x16[(uint8_t)str[i]-32];
            for (int cy = 0; cy < 16; cy++) {
                if (g[cy] == 0) continue;
                for (int dx = 0; dx < 8; dx++) { if (g[cy] & (1<<(7-dx))) { int px = cx + dx, py = y + cy; if (px >= 0 && px < wins[id].w && py >= 0 && py < wins[id].h) wins[id].back_canvas[py * wins[id].w + px] = c; } }
            }
        }
        cx += 8;
    }
}

void gui_draw_dec_win(int id, int x, int y, uint32_t num, uint32_t c) {
    if (id < 0 || id >= MAX_WINS || !wins[id].active) return;
    char buf[16]; uint32_t val = num;
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; gui_draw_text_win(id, x, y, buf, c); return; }
    char temp[16]; int i = 0; while (val > 0) { temp[i++] = (val % 10) + '0'; val /= 10; }
    int j = 0; while (i > 0) { buf[j++] = temp[--i]; } buf[j] = '\0';
    gui_draw_text_win(id, x, y, buf, c);
}

void gui_get_mouse_win(int id, int* mx, int* my, int* clk) {
    if (id < 0 || id >= MAX_WINS || !wins[id].active || wins[id].minimized) { *clk = 0; *mx = -1000; *my = -1000; return; }
    int gx, gy, gmlp; __asm__ volatile("cli"); gx = mouse_x; gy = mouse_y; gmlp = mouse_left_pressed; __asm__ volatile("sti");
    *mx = gx - wins[id].x - 2; *my = gy - wins[id].y - 26; 
    if (gy >= screen_h - 40 || start_menu_open) { *clk = 0; return; }
    int top_id = -1;
    for (int i = win_count - 1; i >= 0; i--) {
        int cid = z_order[i]; Window* cw = &wins[cid];
        if (!cw->minimized && gx >= cw->x && gx <= cw->x + cw->w + 4 && gy >= cw->y && gy <= cw->y + cw->h + 30) { top_id = cid; break; }
    }
    if (top_id == id && !focus_changed_this_click) { *clk = gmlp; } else { *clk = 0; }
}

uint32_t gui_get_state_ptr(int id) { return (uint32_t)&wins[id].state[0]; }

void draw_app_icon(int x, int y, const char* title, int scale) {
    char t0 = title[0]; char t1 = title[1]; char t2 = title[2];
    if (t0 == 'E' || t0 == 'e') { draw_rect(x, y + 4*scale, 20*scale, 14*scale, 0xFFD700); draw_rect(x, y, 10*scale, 4*scale, 0xDAA520); } 
    else if ((t0 == 'S' && t1 == 'Y' && t2 == 'S') || t0 == 'T' || t0 == 't') { draw_rect(x, y, 20*scale, 20*scale, 0x2D2D30); draw_rect(x + 2*scale, y + 2*scale, 16*scale, 16*scale, 0x000000); draw_rect(x + 4*scale, y + 10*scale, 4*scale, 8*scale, 0x00FF00); } 
    else if ((t0 == 'S' && t1 == 'E') || (t0 == 's' && t1 == 'e')) { draw_rect(x + 8*scale, y + 2*scale, 8*scale, 20*scale, 0x888888); draw_rect(x + 2*scale, y + 8*scale, 20*scale, 8*scale, 0x888888); draw_rect(x + 5*scale, y + 5*scale, 14*scale, 14*scale, 0xAAAAAA); } 
    else if (t0 == 'C' || t0 == 'c') { draw_rect(x + 2*scale, y + 2*scale, 16*scale, 16*scale, 0xFFFFFF); draw_rect(x + 9*scale, y + 4*scale, 2*scale, 6*scale, 0x000000); draw_rect(x + 9*scale, y + 9*scale, 6*scale, 2*scale, 0x000000); } 
    else if (t0 == 'V' || t0 == 'v') { draw_rect(x, y, 20*scale, 20*scale, 0x880000); draw_rect(x + 5*scale, y + 5*scale, 10*scale, 10*scale, 0xFF0000); } 
    else if (t0 == 'S' && t1 == 'N') { draw_rect(x, y, 20*scale, 20*scale, 0x005500); draw_rect(x + 4*scale, y + 4*scale, 12*scale, 12*scale, 0x00FF00); draw_rect(x + 10*scale, y + 10*scale, 4*scale, 4*scale, 0xFF0000); }
    else { draw_rect(x, y, 20*scale, 20*scale, 0x00FFFF); }
}

void draw_desktop() {
    int mx, my, mlp; __asm__ volatile("cli"); mx = mouse_x; my = mouse_y; mlp = mouse_left_pressed; __asm__ volatile("sti");
    if (global_wallpaper) { draw_wallpaper(global_wallpaper); } else { draw_rect(0, 0, screen_w, screen_h, 0x1A2A3A); }
    
    int on_desk = 1;
    if (my >= screen_h - 40) { on_desk = 0; } 
    else {
        for (int i = win_count - 1; i >= 0; i--) {
            Window* w = &wins[z_order[i]];
            if (!w->minimized && mx >= w->x && mx <= w->x + w->w + 4 && my >= w->y && my <= w->y + w->h + 30) { on_desk = 0; break; }
        }
        if (start_menu_open) {
            int sm_w = 200, sm_h = 300, sm_x = 5, sm_y = screen_h - 40 - sm_h;
            if (mx >= sm_x && mx <= sm_x + sm_w && my >= sm_y && my <= sm_y + sm_h) { on_desk = 0; }
        }
    }

    int i1_hover = (on_desk && mx >= 30 && mx <= 80 && my >= 30 && my <= 90); if (i1_hover) { draw_rect(25, 25, 60, 65, mlp ? 0x444444 : 0x333333); }
    draw_app_icon(35, 30, "EXPLORER", 2); print_string_at(35, 75, "Files", 0xFFFFFF);

    int i2_hover = (on_desk && mx >= 30 && mx <= 80 && my >= 110 && my <= 170); if (i2_hover) { draw_rect(25, 105, 60, 65, mlp ? 0x444444 : 0x333333); }
    draw_app_icon(35, 110, "SYSINFO", 2); print_string_at(27, 155, "TaskMgr", 0xFFFFFF);

    int i3_hover = (on_desk && mx >= 30 && mx <= 80 && my >= 190 && my <= 250); if (i3_hover) { draw_rect(25, 185, 60, 65, mlp ? 0x444444 : 0x333333); }
    draw_app_icon(35, 190, "SETTINGS", 2); print_string_at(23, 235, "Settings", 0xFFFFFF);

    int i4_hover = (on_desk && mx >= 30 && mx <= 80 && my >= 270 && my <= 330); if (i4_hover) { draw_rect(25, 265, 60, 65, mlp ? 0x444444 : 0x333333); }
    draw_app_icon(35, 270, "CLOCK", 2); print_string_at(30, 315, "Clock", 0xFFFFFF);
    
    draw_alpha_rect(0, screen_h - 40, screen_w, 40, 0x050505, 180); draw_rect(0, screen_h - 40, screen_w, 1, 0x333333); 
    
    int start_hover = 0; if (mx >= 5 && mx <= 90 && my >= screen_h - 35 && my <= screen_h - 5) { start_hover = 1; }
    uint32_t btn_color = 0x2D2D30;
    if (start_hover) { btn_color = 0x0078D7; if (mlp) { btn_color = 0x0055AA; } }
    if (start_menu_open) { btn_color = 0x0078D7; }
    draw_rect(5, screen_h - 35, 85, 30, btn_color); print_string_at(22, screen_h - 27, "START", 0xFFFFFF);

    int h, m, s; get_rtc_time(&h, &m, &s); char tstr[] = "00:00";
    tstr[0] = (h/10)+'0'; tstr[1] = (h%10)+'0'; tstr[3] = (m/10)+'0'; tstr[4] = (m%10)+'0'; print_string_at(screen_w - 60, screen_h - 27, tstr, 0xFFFFFF);

    int tb_x = 100;
    for (int i = 0; i < win_count; i++) {
        int id = z_order[i]; Window* w = &wins[id]; 
        int is_active = (i == win_count - 1 && !w->minimized);
        int tb_hover = (mx >= tb_x && mx <= tb_x + 40 && my >= screen_h - 40);
        uint32_t bg_color = w->minimized ? 0x222222 : (is_active ? 0x444444 : 0x111111);
        
        draw_alpha_rect(tb_x, screen_h - 40, 40, 40, tb_hover ? 0x555555 : bg_color, 180);
        if (is_active) { draw_rect(tb_x + 4, screen_h - 4, 32, 2, 0x0078D7); }
        draw_app_icon(tb_x + 10, screen_h - 30, w->title, 1); tb_x += 44;
    }

    for (int i = 0; i < win_count; i++) {
        int id = z_order[i]; Window* w = &wins[id]; 
        if (!w->active || w->minimized) continue; 
        
        draw_shadow(w->x + 6, w->y + 6, w->w + 4, w->h + 30);
        uint32_t border_color = (i == win_count - 1) ? 0x0078D7 : 0x444444;
        draw_rect(w->x, w->y, w->w + 4, w->h + 30, border_color); 
        draw_rect(w->x + 2, w->y + 2, w->w, w->h + 26, 0x1E1E1E); 
        draw_rect(w->x + 2, w->y + 2, w->w, 24, border_color); 
        print_string_at(w->x + 10, w->y + 6, w->title, 0xFFFFFF);
        
        int min_hover = (mx >= w->x + w->w - 46 && mx <= w->x + w->w - 26 && my >= w->y + 2 && my <= w->y + 26);
        draw_rect(w->x + w->w - 46, w->y + 2, 24, 24, min_hover ? 0x555555 : border_color);
        print_char_at(w->x + w->w - 38, w->y + 6, '_', 0xFFFFFF); 
        
        int close_hover = (mx >= w->x + w->w - 22 && mx <= w->x + w->w - 2 && my >= w->y + 2 && my <= w->y + 26);
        draw_rect(w->x + w->w - 22, w->y + 2, 24, 24, close_hover ? 0xE81123 : border_color); 
        print_char_at(w->x + w->w - 14, w->y + 6, 'X', 0xFFFFFF);
        
        blit_canvas(w->front_canvas, w->x + 2, w->y + 26, w->w, w->h); 
    }

    extern volatile int app_crashed_timer;
    if (app_crashed_timer > 0) {
        draw_rect(0, 0, screen_w, 25, 0xDD0000);
        print_string_at(10, 5, "[!] OS SANDBOX: Application crashed! Process Terminated.", 0xFFFFFF);
        app_crashed_timer--;
    }

    if (start_menu_open) {
        if (!apps_loaded) { fs_get_files(installed_apps); apps_loaded = 1; }
        int sm_w = 200, sm_h = 300, sm_x = 5, sm_y = screen_h - 40 - sm_h;
        draw_alpha_rect(sm_x, sm_y, sm_w, sm_h, 0x1E1E1E, 230);
        draw_rect(sm_x, sm_y, sm_w, 2, 0x0078D7); draw_rect(sm_x, sm_y, 1, sm_h, 0x333333); draw_rect(sm_x + sm_w - 1, sm_y, 1, sm_h, 0x333333);
        print_string_at(sm_x + 15, sm_y + 15, "Installed Programs", 0xAAAAAA); draw_rect(sm_x + 10, sm_y + 35, sm_w - 20, 1, 0x444444);
        int list_y = sm_y + 45; int idx = 0;
        while (installed_apps[idx] && list_y < sm_y + sm_h - 60) {
            char name[16]; int n = 0;
            while (installed_apps[idx] && installed_apps[idx] != '\n') { if (n < 15) { name[n++] = installed_apps[idx]; } idx++; }
            name[n] = '\0'; if (installed_apps[idx] == '\n') { idx++; }
            if (n >= 11 && name[8] == 'E' && name[9] == 'L' && name[10] == 'F') {
                int item_hover = (mx >= sm_x + 5 && mx <= sm_x + sm_w - 5 && my >= list_y && list_y <= sm_y + sm_h - 30);
                if (item_hover) { draw_rect(sm_x + 5, list_y, sm_w - 10, 24, 0x3E3E42); }
                char dname[16]; int di = 0; for (int k = 0; k < 8; k++) { if (name[k] != ' ') { dname[di++] = name[k]; } } dname[di] = '\0';
                draw_app_icon(sm_x + 10, list_y + 2, name, 1); print_string_at(sm_x + 40, list_y + 4, dname, 0xFFFFFF);
                list_y += 30;
            }
        }
        int cli_btn_y = sm_y + sm_h - 35; draw_rect(sm_x + 10, cli_btn_y - 5, sm_w - 20, 1, 0x444444);
        int cli_hover = (mx >= sm_x + 5 && mx <= sm_x + sm_w - 5 && my >= cli_btn_y && my <= cli_btn_y + 30);
        draw_rect(sm_x + 5, cli_btn_y, sm_w - 10, 30, cli_hover ? (mlp ? 0xAA0000 : 0xFF3333) : 0x880000);
        print_string_at(sm_x + 40, cli_btn_y + 7, "Exit to Terminal", 0xFFFFFF);
    }
}

void update_gui() {
    int mx, my, mlp; __asm__ volatile("cli"); mx = mouse_x; my = mouse_y; mlp = mouse_left_pressed; __asm__ volatile("sti"); 
    int click = mlp && !prev_click; if (!mlp) { focus_changed_this_click = 0; drag_win = -1; }
    
    if (click || mlp != prev_click || drag_win != -1 || start_menu_open) { gui_needs_redraw = 1; mark_dirty(0, 0, screen_w, screen_h); }

    static int last_s = -1; int h, m, s; get_rtc_time(&h, &m, &s); 
    if (s != last_s) { gui_needs_redraw = 1; mark_dirty(screen_w - 65, screen_h - 30, 60, 20); last_s = s; }

    if (click) {
        if (mx >= 5 && mx <= 90 && my >= screen_h - 35 && my <= screen_h - 5) { start_menu_open = !start_menu_open; prev_click = mlp; return; }
        if (start_menu_open) {
            int sm_w = 200, sm_h = 300, sm_x = 5, sm_y = screen_h - 40 - sm_h;
            if (mx >= sm_x && mx <= sm_x + sm_w && my >= sm_y && my <= sm_y + sm_h) {
                int cli_btn_y = sm_y + sm_h - 35;
                if (mx >= sm_x + 5 && mx <= sm_x + sm_w - 5 && my >= cli_btn_y && my <= cli_btn_y + 30) { extern void exit_desktop(void); start_menu_open = 0; prev_click = mlp; exit_desktop(); return; }
                int list_y = sm_y + 45, idx = 0;
                while (installed_apps[idx] && list_y < sm_y + sm_h - 45) {
                    char name[16]; int n = 0; while (installed_apps[idx] && installed_apps[idx] != '\n') { if (n < 15) { name[n++] = installed_apps[idx]; } idx++; } name[n] = '\0'; if (installed_apps[idx] == '\n') idx++;
                    if (n >= 11 && name[8] == 'E' && name[9] == 'L' && name[10] == 'F') { if (mx >= sm_x + 5 && mx <= sm_x + sm_w - 5 && my >= list_y && my <= list_y + 24) { exec_app(name); start_menu_open = 0; prev_click = mlp; return; } list_y += 30; }
                }
                prev_click = mlp; return; 
            } else { start_menu_open = 0; }
        }

        if (my >= screen_h - 40) {
            int tb_x = 100;
            for (int i = 0; i < win_count; i++) {
                if (mx >= tb_x && mx <= tb_x + 40) { 
                    int id = z_order[i]; 
                    if (i == win_count - 1 && !wins[id].minimized) { wins[id].minimized = 1; } 
                    else { wins[id].minimized = 0; for (int j = i; j < win_count - 1; j++) { z_order[j] = z_order[j+1]; } z_order[win_count - 1] = id; }
                    gui_needs_redraw = 1; mark_dirty(0, 0, screen_w, screen_h); prev_click = mlp; return; 
                } 
                tb_x += 44;
            } return;
        }

        for (int i = win_count - 1; i >= 0; i--) {
            int id = z_order[i]; Window* w = &wins[id];
            if (!w->minimized && mx >= w->x && mx <= w->x + w->w + 4 && my >= w->y && my <= w->y + w->h + 30) {
                if (i != win_count - 1) { focus_changed_this_click = 1; }
                for (int j = i; j < win_count - 1; j++) { z_order[j] = z_order[j+1]; } z_order[win_count - 1] = id;
                
                if (mx >= w->x + w->w - 22 && mx <= w->x + w->w - 2 && my <= w->y + 26) { w->active = 0; kfree(w->front_canvas); kfree(w->back_canvas); win_count--; mark_dirty(0, 0, screen_w, screen_h); }
                else if (mx >= w->x + w->w - 46 && mx <= w->x + w->w - 26 && my <= w->y + 26) { w->minimized = 1; mark_dirty(0, 0, screen_w, screen_h); }
                else if (my <= w->y + 26) { drag_win = id; drag_off_x = mx - w->x; drag_off_y = my - w->y; } 
                prev_click = mlp; return; 
            }
        }
    }
    if (drag_win != -1) { wins[drag_win].x = mx - drag_off_x; wins[drag_win].y = my - drag_off_y; gui_needs_redraw = 1; mark_dirty(0, 0, screen_w, screen_h); } 
    prev_click = mlp;
}

void reset_desktop() { for (int i = 0; i < MAX_WINS; i++) { if (wins[i].active) { kfree(wins[i].front_canvas); kfree(wins[i].back_canvas); } wins[i].active = 0; } win_count = 0; drag_win = -1; prev_click = 0; spawn_offset = 0; apps_loaded = 0; start_menu_open = 0; mark_dirty(0,0,screen_w,screen_h); }