#include "task.h"
#include "memory.h"
#include "graphics.h" // Для отладочного вывода

// Структура стека, как её оставляет процессор и команда pusha после прерывания
typedef struct {
    unsigned int edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; // Сохранено pusha
    unsigned int eip, cs, eflags;                              // Сохранено процессором аппаратно
} task_regs_t;

// Хранилище задач (пока максимум 4 одновременно: 0 - Ядро, 1-3 - Программы)
unsigned int task_esps[4];
int num_tasks = 1;     // По умолчанию работает только 1 задача (Ядро)
int current_task = 0;

unsigned int timer_ticks = 0; // ТЕПЕРЬ ТАЙМЕР ЖИВЕТ ЗДЕСЬ!

// ПЛАНИРОВЩИК (Вызывается 100 раз в секунду из Ассемблера)
unsigned int schedule(unsigned int current_esp) {
    timer_ticks++; // Не забываем обновлять системное время!

    // Если задача всего одна (Ядро), переключать нечего
    if (num_tasks <= 1) return current_esp;

    // 1. Сохраняем ESP текущей задачи
    task_esps[current_task] = current_esp;

    // 2. Переключаемся на следующую задачу (Round Robin)
    current_task++;
    if (current_task >= num_tasks) current_task = 0;

    // 3. Отдаем Ассемблеру ESP новой задачи
    return task_esps[current_task];
}

// СОЗДАНИЕ НОВОЙ ЗАДАЧИ
void create_task(void (*entry_point)()) {
    if (num_tasks >= 4) return; // Нет места

    // 1. Выделяем 4 Килобайта памяти под личный Стек новой программы
    unsigned int stack_memory = kmalloc(4096);
    
    // Стек растет ВНИЗ. Указываем на конец выделенной памяти
    unsigned int stack_top = stack_memory + 4096;

    // 2. Спускаемся вниз на размер структуры регистров
    task_regs_t* regs = (task_regs_t*)(stack_top - sizeof(task_regs_t));

    // 3. ИСКУССТВЕННО ЗАПОЛНЯЕМ СТЕК, будто программу прервали на первой же строчке!
    regs->eax = 0; regs->ebx = 0; regs->ecx = 0; regs->edx = 0;
    regs->esi = 0; regs->edi = 0; regs->ebp = 0;
    
    regs->eip = (unsigned int) entry_point; // Куда должен прыгнуть процессор (Адрес программы)
    regs->cs  = 0x08;                       // Сегмент кода ядра
    regs->eflags = 0x202;                   // Стандартные флаги (Прерывания включены)

    // 4. Сохраняем ESP этой новой задачи в список
    task_esps[num_tasks] = (unsigned int) regs;
    num_tasks++;
}