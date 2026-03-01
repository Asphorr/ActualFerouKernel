#include "libc.h"
void _start() { 
    int win = create_window("Task Manager", 300, 200); 
    while (is_window_open(win)) {
        draw_rect(win, 0, 0, 300, 200, 0x1E1E1E); draw_text(win, 10, 10, "NexaOS Kernel Info", 0x00FFFF); draw_rect(win, 10, 30, 280, 1, 0x555555);
        draw_text(win, 10, 45, "Architecture: x86 32-bit", 0xFFFFFF); draw_text(win, 10, 65, "Scheduler: PREEMPTIVE MMU", 0x00FF00);
        uint32_t total = get_total_mem() / 1024; uint32_t used = get_used_mem() / 1024;
        draw_text(win, 10, 95, "Total RAM:", 0xAAAAAA); draw_dec(win, 100, 95, total, 0xFFFFFF); draw_text(win, 160, 95, "KB", 0xFFFFFF);
        draw_text(win, 10, 115, "Used RAM:", 0xAAAAAA); draw_dec(win, 100, 115, used, 0xFF5555); draw_text(win, 160, 115, "KB", 0xFF5555);
        draw_text(win, 10, 145, "System Ticks:", 0xAAAAAA); draw_dec(win, 120, 145, get_ticks(), 0x00A2ED);
        
        update_window(win); // <--- КОММИТ КАДРА! Без него окно будет черным
        sleep_ms(50);
    }
}