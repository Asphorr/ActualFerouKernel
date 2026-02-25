#include "syscalls.h"
#include "graphics.h"
#include "gui.h"
#include "util.h" // Для int_to_string и str_length

extern Window windows[MAX_WINDOWS];
extern int num_tasks;
extern int current_task;
extern unsigned int task_esps[4];
extern unsigned int timer_ticks; 
extern unsigned int free_mem_addr;

void syscall_handler_c(registers_t* regs) {
    switch (regs->eax) {
        case 0: // SYS_EXIT 
            if (num_tasks > 1 && current_task == 1) {
                num_tasks = 1;      
                current_task = 0;   
                
                extern int gui_active;
                if (!gui_active) {
                    clear_graphics(0x00);
                    draw_string("=== Program Terminated ===\n", 0x0F);
                    swap_buffers();
                }

                __asm__ volatile ("mov %0, %%esp \n\t popa \n\t iret \n\t" : : "r" (task_esps[0]) : "memory");
            }
            break;

        case 1: // SYS_CLEAR
            clear_graphics(regs->ebx); swap_buffers();
            break;

        case 2: // SYS_DRAW_RECT
            draw_rectangle(regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi); swap_buffers();
            break;

        case 3: // SYS_CREATE_WIN
            regs->eax = create_window(regs->ebx, regs->ecx, regs->edx, regs->esi, (char*)regs->edi);
            break;

        case 4: // SYS_SET_CONTENT (Очистить окно и вставить текст)
            if (regs->ebx >= 0 && regs->ebx < MAX_WINDOWS && windows[regs->ebx].is_open) {
                char* text = (char*)regs->esi; int i = 0;
                while(text[i] != '\0' && i < 127) { windows[regs->ebx].content[i] = text[i]; i++; }
                windows[regs->ebx].content[i] = '\0'; 
            }
            break;

        case 5: // SYS_GET_UPTIME
            regs->eax = timer_ticks / 100; 
            break;

        case 6: // SYS_GET_RAM
            regs->eax = free_mem_addr - 0x100000;
            break;

        // --- НОВЫЕ СИСКОЛЛЫ ДЛЯ УДОБСТВА ПРОГРАММ ---
        case 7: // SYS_ITOA (Перевод числа EBX в строку по адресу ECX)
            int_to_string(regs->ebx, (char*)regs->ecx);
            break;

        case 8: // SYS_APPEND_CONTENT (Дописать текст ESI в окно EBX)
            if (regs->ebx >= 0 && regs->ebx < MAX_WINDOWS && windows[regs->ebx].is_open) {
                char* text = (char*)regs->esi;
                int len = str_length(windows[regs->ebx].content);
                int i = 0;
                while(text[i] != '\0' && len < 127) {
                    windows[regs->ebx].content[len++] = text[i++];
                }
                windows[regs->ebx].content[len] = '\0';
            }
            break;
            
        default: break;
    }
}