#include "libc.h"

void _start() {
    int win = create_window("Malware Test", 300, 150);
    
    draw_rect(win, 0, 0, 300, 150, 0x1E1E1E);
    draw_text(win, 10, 10, "Attempting to hack Kernel...", 0xFF5555);
    draw_text(win, 10, 30, "Target address: 0x00100000", 0xAAAAAA);
    update_window(win);
    sleep_ms(1500); // Даем пользователю прочитать текст

    // ЗЛОВРЕДНЫЙ КОД:
    // Пытаемся записать мусор (0xDEADBEEF) прямо в начало ядра системы!
    // Так как процессор сейчас находится в бесправном Ring 3, 
    // аппаратный блок MMU должен распознать вторжение, заблокировать шину 
    // и выбросить прерывание Page Fault (Ошибка защиты страницы).
    volatile uint32_t* kernel_memory = (uint32_t*)0x100000;
    *kernel_memory = 0xDEADBEEF; 

    // Если код дошел до сюда — Ядро проиграло битву.
    draw_text(win, 10, 60, "FATAL: OS HAS BEEN COMPROMISED!", 0xFF0000);
    update_window(win);
    
    while(is_window_open(win)) {
        sleep_ms(100);
    }
}