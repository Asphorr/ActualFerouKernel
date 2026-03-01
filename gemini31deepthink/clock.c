#include "libc.h"

void _start() {
    int win = create_window("Clock", 200, 100);
    while (is_window_open(win)) {
        draw_rect(win, 0, 0, 200, 100, 0x1E1E1E); 
        draw_rect(win, 10, 20, 180, 60, 0x050505); draw_rect(win, 10, 20, 180, 2, 0x333333);
        
        uint32_t t = get_ticks(); char ms[10]; to_string(t, ms);
        draw_text(win, 20, 40, "Ticks:", 0xAAAAAA); draw_text(win, 80, 40, ms, 0x00FF00);
        
        // Рассылаем свое время по IPC Диспетчеру задач!
        int target_pid = get_pid_by_name("SYSINFO ELF");
        if (target_pid > 0) { ipc_send(target_pid, 99, t, 0); }
        
        update_window(win); sleep_ms(16); 
    }
}