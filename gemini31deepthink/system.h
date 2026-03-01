#pragma once

typedef unsigned char uint8_t; typedef unsigned short uint16_t;
typedef unsigned int uint32_t; typedef unsigned int size_t; typedef char int8_t;
#define NULL ((void*)0)

static inline void outb(uint16_t port, uint8_t val) { __asm__ volatile("outb %0,%1" : : "a"(val), "Nd"(port)); }
static inline uint8_t inb(uint16_t port) { uint8_t r; __asm__ volatile("inb %1,%0" : "=a"(r) : "Nd"(port)); return r; }
static inline uint16_t inw(uint16_t port) { uint16_t r; __asm__ volatile("inw %1,%0" : "=a"(r) : "Nd"(port)); return r; }
static inline void outw(uint16_t port, uint16_t val) { __asm__ volatile("outw %0,%1" : : "a"(val), "Nd"(port)); }
int strcmp(const char *s1, const char *s2);

static inline int strlen(const char *s) { int len = 0; while (s[len]) len++; return len; }

static inline void fast_memcpy(void *dst, const void *src, uint32_t size_bytes) {
    uint8_t *d = (uint8_t*)dst; const uint8_t *s = (const uint8_t*)src;
    while (size_bytes >= 16) { __asm__ volatile("movdqu (%0), %%xmm0\n\tmovdqu %%xmm0, (%1)" :: "r"(s), "r"(d) : "memory", "xmm0"); d += 16; s += 16; size_bytes -= 16; }
    while (size_bytes--) { *d++ = *s++; }
}
static inline void fast_fill32(void *dst, uint32_t val, uint32_t count) {
    uint32_t *d = (uint32_t*)dst;
    if (count >= 4) { __asm__ volatile("movd %0, %%xmm0\n\tpshufd $0, %%xmm0, %%xmm0" :: "r"(val) : "memory", "xmm0"); while (count >= 4) { __asm__ volatile("movdqu %%xmm0, (%0)" :: "r"(d) : "memory"); d += 4; count -= 4; } }
    while (count--) { *d++ = val; }
}

#define MEM_MAGIC 0x1BADB002
typedef struct mem_block { uint32_t magic; size_t size; uint32_t is_free; struct mem_block *next; struct mem_block *prev; uint32_t pad[3]; } mem_block_t;

typedef struct { uint8_t e_ident[16]; uint16_t e_type; uint16_t e_machine; uint32_t e_version; uint32_t e_entry; uint32_t e_phoff; uint32_t e_shoff; uint32_t e_flags; uint16_t e_ehsize; uint16_t e_phentsize; uint16_t e_phnum; uint16_t e_shentsize; uint16_t e_shnum; uint16_t e_shstrndx; } __attribute__((packed)) Elf32_Ehdr;
typedef struct { uint32_t p_type; uint32_t p_offset; uint32_t p_vaddr; uint32_t p_paddr; uint32_t p_filesz; uint32_t p_memsz; uint32_t p_flags; uint32_t p_align; } __attribute__((packed)) Elf32_Phdr;
typedef struct { uint32_t sender_pid; uint32_t type; uint32_t data1; uint32_t data2; } IPC_Message;

extern uint32_t kernel_page_directory[1024]; extern volatile uint32_t current_task_cr3;
void init_paging(void); void init_memory(void); void *kmalloc(size_t size); void kfree(void *ptr); uint32_t get_used_memory(void); 
extern uint16_t screen_w, screen_h; extern int cursor_x, cursor_y, input_start_x, gui_mode;
extern volatile uint8_t key_ready, mouse_moved, mouse_left_pressed; extern volatile char key_buffer; extern volatile int mouse_x, mouse_y;
extern volatile uint8_t app_key_ready; extern volatile char app_key_buffer; extern uint8_t* global_wallpaper;

extern volatile int gui_needs_redraw; extern int dirty_min_x, dirty_min_y, dirty_max_x, dirty_max_y; 
void mark_dirty(int x, int y, int w, int h);
void init_pat(void); void set_vram_wc(uint32_t lfb_addr, uint32_t size); void init_sse(void); 

// ИНТЕРФЕЙСЫ TSS
void set_kernel_stack(uint32_t stack);
extern uint32_t stack_top;

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

void process_manager_init(void); void exec_app(const char* name); void set_task_zombie(void); void kill_all_tasks(void);