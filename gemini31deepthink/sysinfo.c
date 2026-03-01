#include "libc.h"

void _start() { 
    int win = create_window("Task Manager", 300, 240); 
    int my_pid = get_pid();
    uint32_t last_sender = 0, last_type = 0, last_d1 = 0;
    
    while (is_window_open(win)) {
        draw_rect(win, 0, 0, 300, 240, 0x1E1E1E); 
        draw_text(win, 10, 10, "NexaOS Kernel Info", 0x00FFFF); 
        draw_rect(win, 10, 30, 280, 1, 0x555555);
        
        draw_text(win, 10, 45, "PID:", 0xAAAAAA); 
        draw_dec(win, 50, 45, my_pid, 0x00FF00);
        draw_text(win, 10, 65, "Format: ELF32 (Virtual MMU)", 0x00FF00); 
        
        uint32_t total = get_total_mem() / 1024; uint32_t used = get_used_mem() / 1024;
        draw_text(win, 10, 95, "Total RAM:", 0xAAAAAA); draw_dec(win, 100, 95, total, 0xFFFFFF); draw_text(win, 160, 95, "KB", 0xFFFFFF);
        draw_text(win, 10, 115, "Used RAM:", 0xAAAAAA); draw_dec(win, 100, 115, used, 0xFF5555); draw_text(win, 160, 115, "KB", 0xFF5555);
        
        // Читаем почтовый ящик IPC!
        ipc_msg_t msg;
        if (ipc_recv(&msg)) {
            last_sender = msg.sender_pid; last_type = msg.type; last_d1 = msg.data1;
        }
        
        draw_rect(win, 10, 145, 280, 1, 0x555555); draw_text(win, 10, 155, "IPC Mailbox (Inbox):", 0x00A2ED);
        if (last_sender != 0) {
            draw_text(win, 10, 175, "From PID:", 0xAAAAAA); draw_dec(win, 90, 175, last_sender, 0xFFFFFF);
            draw_text(win, 10, 195, "Cmd Type:", 0xAAAAAA); draw_dec(win, 90, 195, last_type, 0xFFFFFF);
            draw_text(win, 10, 215, "Data:", 0xAAAAAA); draw_dec(win, 90, 215, last_d1, 0xFFFFFF);
        } else {
            draw_text(win, 10, 175, "No messages received.", 0x555555);
        }
        
        update_window(win); sleep_ms(50);
    }
}