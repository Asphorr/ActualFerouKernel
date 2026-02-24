#include "memory.h"

// НОВОЕ: Начинаем выдавать память с отметки 1 Мегабайт!
unsigned int free_mem_addr = 0x100000;

unsigned int kmalloc(unsigned int size) {
    unsigned int ret = free_mem_addr;
    free_mem_addr += size;
    return ret;
}