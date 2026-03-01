#pragma once
typedef unsigned int uint32_t; typedef unsigned int size_t; typedef unsigned char uint8_t;

static inline uint32_t syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx, uint32_t esi) {
    uint32_t ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(eax), "b"(ebx), "c"(ecx), "d"(edx), "S"(esi) : "memory"); return ret;
}

static inline int create_window(const char* title, int w, int h) { return syscall(1, (uint32_t)title, w, h, 0); }
static inline void draw_rect(int win, int x, int y, int w, int h, uint32_t color) { syscall(2, win, (x << 16) | (y & 0xFFFF), (w << 16) | (h & 0xFFFF), color); }
static inline void draw_text(int win, int x, int y, const char* str, uint32_t color) { syscall(3, win, (x << 16) | (y & 0xFFFF), (uint32_t)str, color); }
static inline void get_mouse(int win, int* x, int* y, int* clicks) { syscall(4, win, (uint32_t)x, (uint32_t)y, (uint32_t)clicks); }
static inline void get_files(char* buf) { syscall(5, (uint32_t)buf, 0, 0, 0); }
static inline void exec(const char* name) { syscall(6, (uint32_t)name, 0, 0, 0); }
static inline uint32_t get_ticks() { return syscall(7, 0, 0, 0, 0); }
static inline uint32_t get_used_mem() { return syscall(9, 0, 0, 0, 0); }
static inline uint32_t get_total_mem() { return syscall(10, 0, 0, 0, 0); }
static inline void set_wallpaper(const char* name) { syscall(11, (uint32_t)name, 0, 0, 0); } 
static inline int file_write(const char* name, void* buf, uint32_t size) { return syscall(12, (uint32_t)name, (uint32_t)buf, size, 0); }
static inline char get_key(int win) { return syscall(13, win, 0, 0, 0); }

// ИСПРАВЛЕНИЕ: Собственный безопасный Allocator для Ring 3! (Выделяет память в разрешенной зоне 1 Мегабайт)
static uint8_t* user_heap_ptr = (uint8_t*)0x80100000; 
static inline void* malloc(size_t size) { void* p = user_heap_ptr; user_heap_ptr += (size + 15) & ~15; return p; }
static inline void free(void* ptr) { (void)ptr; } 

static inline void draw_alpha(int win, int x, int y, int w, int h, uint32_t color, uint8_t alpha) { syscall(16, win, (x << 16) | (y & 0xFFFF), (w << 16) | (h & 0xFFFF), (color & 0xFFFFFF) | (alpha << 24)); }
static inline uint32_t rand(void) { return syscall(18, 0, 0, 0, 0); }
static inline void draw_dec(int win, int x, int y, uint32_t num, uint32_t color) { syscall(19, win, (x << 16) | (y & 0xFFFF), num, color); }
static inline int is_focused(int win) { return syscall(20, win, 0, 0, 0); }
static inline int is_window_open(int win) { return syscall(22, win, 0, 0, 0); }
static inline void sleep_ms(uint32_t ms) { syscall(23, ms, 0, 0, 0); }
static inline void update_window(int win) { syscall(24, win, 0, 0, 0); }
static inline void exit_app() { syscall(25, 0, 0, 0, 0); }
static inline void yield() { sleep_ms(1); } // int 0x81 теперь запрещен для Ring 3, заменяем безопасным слипом

typedef struct { uint32_t sender_pid; uint32_t type; uint32_t data1; uint32_t data2; } ipc_msg_t;
static inline int get_pid() { return syscall(26, 0, 0, 0, 0); }
static inline int ipc_send(int dest_pid, uint32_t type, uint32_t d1, uint32_t d2) { return syscall(27, dest_pid, type, d1, d2); }
static inline int ipc_recv(ipc_msg_t* msg) { return syscall(28, (uint32_t)msg, 0, 0, 0); }
static inline int get_pid_by_name(const char* name) { return syscall(29, (uint32_t)name, 0, 0, 0); }

static inline void* memset(void* dest, int val, size_t count) { uint8_t* p = (uint8_t*)dest; while (count--) { *p++ = (uint8_t)val; } return dest; }
static inline void* memcpy(void* dest, const void* src, size_t count) { uint8_t* d = (uint8_t*)dest; const uint8_t* s = (const uint8_t*)src; while (count--) { *d++ = *s++; } return dest; }
static inline size_t strlen(const char* str) { size_t len = 0; while (str[len]) len++; return len; }
static inline char* strcpy(char* dest, const char* src) { char* d = dest; while ((*d++ = *src++)); return dest; }

static inline void to_string(uint32_t val, char* buf) {
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char temp[16]; int i = 0; while (val > 0) { temp[i++] = (val % 10) + '0'; val /= 10; }
    int j = 0; while (i > 0) { buf[j++] = temp[--i]; } buf[j] = '\0';
}