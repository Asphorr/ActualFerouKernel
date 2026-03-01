#pragma once

typedef unsigned char uint8_t; typedef unsigned short uint16_t;
typedef unsigned int uint32_t; typedef unsigned int size_t; typedef char int8_t;
#define NULL ((void*)0)

static inline void outb(uint16_t port, uint8_t val) { __asm__ volatile("outb %0,%1" : : "a"(val), "Nd"(port)); }
static inline uint8_t inb(uint16_t port) { uint8_t r; __asm__ volatile("inb %1,%0" : "=a"(r) : "Nd"(port)); return r; }
static inline uint16_t inw(uint16_t port) { uint16_t r; __asm__ volatile("inw %1,%0" : "=a"(r) : "Nd"(port)); return r; }
static inline void outw(uint16_t port, uint16_t val) { __asm__ volatile("outw %0,%1" : : "a"(val), "Nd"(port)); }
int strcmp(const char *s1, const char *s2);

static inline void fast_memcpy(void *dst, const void *src, uint32_t size_bytes) {
    uint32_t d0, d1, d2; __asm__ __volatile__("cld\n\trep movsl\n\tmov %4, %%ecx\n\trep movsb" : "=&c"(d0), "=&D"(d1), "=&S"(d2) : "0"(size_bytes / 4), "r"(size_bytes % 4), "1"(dst), "2"(src) : "memory", "cc");
}
static inline void fast_fill32(void *dst, uint32_t val, uint32_t count) {
    uint32_t d0, d1; __asm__ __volatile__("cld\n\trep stosl" : "=&c"(d0), "=&D"(d1) : "0"(count), "1"(dst), "a"(val) : "memory", "cc");
}

#define MEM_MAGIC 0x1BADB002
typedef struct mem_block { uint32_t magic; size_t size; uint8_t is_free; struct mem_block *next; struct mem_block *prev; } __attribute__((packed)) mem_block_t;

// Экспортируем Каталог Ядра и регистр CR3 для переключения пространств
extern uint32_t kernel_page_directory[1024];
extern volatile uint32_t current_task_cr3;

void init_paging(void); void init_memory(void); void *kmalloc(size_t size); void kfree(void *ptr); uint32_t get_used_memory(void); 
extern uint16_t screen_w, screen_h; extern int cursor_x, cursor_y, input_start_x, gui_mode;
extern volatile uint8_t key_ready, mouse_moved, mouse_left_pressed; extern volatile char key_buffer; extern volatile int mouse_x, mouse_y;
extern volatile uint8_t app_key_ready; extern volatile char app_key_buffer; extern uint8_t* global_wallpaper;

void init_graphics(void); void draw_rect(int x, int y, int w, int h, uint32_t color); void draw_shadow(int x, int y, int w, int h); void draw_wallpaper(uint8_t* image); 
void draw_alpha_rect(int x, int y, int w, int h, uint32_t color, uint8_t alpha); void print_char(char c, uint32_t color); void print_string(const char* str, uint32_t color);
void print_hex(uint32_t num, uint32_t color); void print_dec(uint32_t num, uint32_t color); void swap_buffers(void); void put_pixel_buf(int x, int y, uint32_t color);
void print_char_at(int x, int y, char c, uint32_t color); void print_string_at(int x, int y, const char* str, uint32_t color); void blit_canvas(uint32_t* canvas, int x, int y, int w, int h); 

extern volatile uint32_t sys_ticks; extern volatile int app_crashed_timer;
void init_gdt(void); void init_hardware(void); void get_rtc_time(int* h, int* m, int* s); 
void fs_init(void); void* load_fat16_file(const char* filename, uint32_t* out_size); void fs_write_file(const char* target_name, uint8_t* data, uint32_t size); void fs_get_files(char* out_buf); 

void reset_desktop(void); void enter_desktop(void); void exit_desktop(void); void update_gui(void); void draw_desktop(void);
int gui_register_window(const char* title, int w, int h); void gui_draw_rect_win(int id, int x, int y, int w, int h, uint32_t color);
void gui_draw_alpha_win(int id, int x, int y, int w, int h, uint32_t arg); void gui_draw_text_win(int id, int x, int y, const char* str, uint32_t color);
void gui_draw_dec_win(int id, int x, int y, uint32_t num, uint32_t c); void gui_get_mouse_win(int id, int* mx, int* my, int* clicks);
void gui_swap_window_buffers(int id); uint32_t gui_get_state_ptr(int id); int gui_is_focused(int id); int gui_is_window_open(int id);

void process_manager_init(void); void exec_app(const char* name); void set_task_zombie(void);