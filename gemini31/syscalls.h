#pragma once

typedef struct {
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
} registers_t;

void syscall_handler_c(registers_t* regs);

// Команды Syscall
#define SYS_EXIT         0
#define SYS_CLEAR        1
#define SYS_DRAW_RECT    2
#define SYS_CREATE_WIN   3  // Создать окно
#define SYS_DRAW_TEXT    4  // Написать текст в окне
#define SYS_GET_UPTIME   5  // Получить время работы
#define SYS_GET_RAM      6  // Получить объем памяти