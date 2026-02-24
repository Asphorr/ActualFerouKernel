#include "syscalls.h"
#include "graphics.h"

extern int num_tasks;
extern int current_task;
extern unsigned int task_esps[4];

void syscall_handler_c(registers_t* regs) {
    switch (regs->eax) {
        case 0: // SYS_EXIT
            // Убиваем текущую задачу (у нас пока только одна программа, индекс 1)
            if (num_tasks > 1 && current_task == 1) {
                num_tasks = 1;      // Теперь работает только Ядро
                current_task = 0;   // Переключаем активную задачу на Ядро

                // Очищаем экран, чтобы стереть художества программы
                clear_graphics(0x00);
                draw_string("=== Program Terminated ===\n", 0x0F);
                swap_buffers();

                // МАГИЯ: Мы подменяем сохраненный стек ПРЯМО В ПРЕРЫВАНИИ!
                // Когда ассемблер сделает 'popa' и 'iretd', он возьмет данные из стека Ядра,
                // а не из стека убитой программы. Ядро проснется ровно там, где его прервали!
                __asm__ volatile (
                    "mov %0, %%esp \n\t"  // Загружаем ESP Ядра (task_esps[0])
                    "popa \n\t"           // Восстанавливаем регистры Ядра
                    "iret \n\t"          // ВЫХОД ИЗ ПРЕРЫВАНИЯ в код Ядра!
                    : 
                    : "r" (task_esps[0])  // Передаем ESP Ядра как аргумент
                    : "memory"
                );
            }
            break;

        case 1: // SYS_CLEAR
            clear_graphics(regs->ebx);
            swap_buffers();
            break;

        case 2: // SYS_DRAW_RECT
            draw_rectangle(regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
            swap_buffers();
            break;
            
        default: break;
    }
}