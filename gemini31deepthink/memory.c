#include "system.h"

#define HEAP_START 0x1000000 
#define HEAP_SIZE  (32 * 1024 * 1024) 
static mem_block_t *head = NULL;

void init_memory(void) {
    head = (mem_block_t*)HEAP_START; head->magic = MEM_MAGIC; head->size = HEAP_SIZE - sizeof(mem_block_t);
    head->is_free = 1; head->next = NULL; head->prev = NULL;
}

uint32_t kernel_page_directory[1024] __attribute__((aligned(4096)));

void init_paging(void) {
    uint32_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= 0x00000010; 
    __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));

    for (uint32_t i = 0; i < 1024; i++) {
        // ИСПРАВЛЕНИЕ ЗАЩИТЫ RING 3:
        // 0x83 = 4MB(1), Supervisor Only(0), Read/Write(1), Present(1)
        // Любая программа, обратившаяся сюда, получит Page Fault!
        kernel_page_directory[i] = (i * 0x400000U) | 0x83; 
    }
    
    __asm__ volatile ("mov %0, %%cr3\n\tmov %%cr0, %%eax\n\tor $0x80000000, %%eax\n\tmov %%eax, %%cr0\n\t" :: "r"(kernel_page_directory) : "eax", "memory");
}

void set_vram_wc(uint32_t lfb_addr, uint32_t size) {
    uint32_t start_pde = lfb_addr / 0x400000U;
    uint32_t end_pde = (lfb_addr + size + 0x3FFFFFU) / 0x400000U;
    for (uint32_t i = start_pde; i <= end_pde; i++) {
        if (i < 1024) { kernel_page_directory[i] |= 0x08; }
    }
    uint32_t cr3; __asm__ volatile("mov %%cr3, %0\n\tmov %0, %%cr3" : "=r"(cr3));
}

void *kmalloc(size_t size) {
    if (size == 0) { return NULL; }
    size = (size + 15) & ~15; 
    uint32_t flags; __asm__ volatile("pushf\n\tpop %0\n\tcli" : "=r"(flags));
    
    mem_block_t *curr = head; void *ret_ptr = NULL;
    while (curr != NULL) {
        if (curr->magic != MEM_MAGIC) { break; }
        if (curr->is_free && curr->size >= size) {
            if (curr->size >= size + sizeof(mem_block_t) + 16) {
                mem_block_t *new_block = (mem_block_t*)((uint8_t*)curr + sizeof(mem_block_t) + size);
                new_block->magic = MEM_MAGIC; new_block->size = curr->size - size - sizeof(mem_block_t);
                new_block->is_free = 1; new_block->next = curr->next; new_block->prev = curr;
                if (curr->next != NULL) { curr->next->prev = new_block; }
                curr->next = new_block; curr->size = size;
            }
            curr->is_free = 0; ret_ptr = (void*)((uint8_t*)curr + sizeof(mem_block_t)); break;
        }
        curr = curr->next;
    }
    if (ret_ptr) { fast_fill32(ret_ptr, 0, size / 4); }
    if (flags & 0x200) { __asm__ volatile("sti"); } return ret_ptr;
}

void kfree(void *ptr) {
    if (ptr == NULL) { return; }
    uint32_t flags; __asm__ volatile("pushf\n\tpop %0\n\tcli" : "=r"(flags));
    mem_block_t *block = (mem_block_t*)((uint8_t*)ptr - sizeof(mem_block_t));
    if (block->magic == MEM_MAGIC) {
        block->is_free = 1;
        if (block->next != NULL && block->next->is_free && block->next->magic == MEM_MAGIC) {
            block->size += sizeof(mem_block_t) + block->next->size; block->next = block->next->next;
            if (block->next != NULL) { block->next->prev = block; }
        }
        if (block->prev != NULL && block->prev->is_free && block->prev->magic == MEM_MAGIC) {
            block->prev->size += sizeof(mem_block_t) + block->size; block->prev->next = block->next;
            if (block->next != NULL) { block->next->prev = block->prev; }
        }
    }
    if (flags & 0x200) { __asm__ volatile("sti"); }
}

uint32_t get_used_memory(void) {
    uint32_t used = 0; mem_block_t *curr = head;
    while (curr != NULL) { if (!curr->is_free) { used += curr->size + sizeof(mem_block_t); } curr = curr->next; } return used;
}