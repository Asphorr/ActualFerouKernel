#pragma once
void create_task(void (*entry_point)()); // Создать новый процесс
unsigned int schedule(unsigned int current_esp); // Планировщик (вызывается таймером)