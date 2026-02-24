#pragma once

// Структура, которая в точности повторяет то, как инструкция 'pusha' кладет регистры в стек
typedef struct {
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
} registers_t;

void syscall_handler_c(registers_t* regs);