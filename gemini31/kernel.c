#include "ports.h"
#include "graphics.h"
#include "mouse.h"
#include "shell.h"
#include "fat16.h"
#include "syscalls.h"
#include "gui.h"

// ==========================================
// СИСТЕМНЫЕ СТРУКТУРЫ (CPU / IDT)
// ==========================================
typedef struct {
    unsigned short low_offset;   unsigned short selector;     
    unsigned char  always0;      unsigned char  flags;        
    unsigned short high_offset;  
} __attribute__((packed)) idt_gate_t;

typedef struct {
    unsigned short limit;        unsigned int base;           
} __attribute__((packed)) idt_register_t;

idt_gate_t idt[256];
idt_register_t idt_reg;

extern void load_idt(unsigned int idt_ptr);
extern void isr0_wrapper();  
extern void isr1_wrapper();  
extern void irq0_wrapper();  
extern void irq12_wrapper(); 
extern void isr128_wrapper(); 

extern unsigned int timer_ticks;

void set_idt_gate(int n, unsigned int handler) {
    idt[n].low_offset = handler & 0xFFFF; idt[n].selector = 0x08;
    idt[n].always0 = 0; idt[n].flags = 0x8E; idt[n].high_offset = (handler >> 16) & 0xFFFF;
}

void init_timer() { port_byte_out(0x43, 0x36); port_byte_out(0x40, 11931 & 0xFF); port_byte_out(0x40, 11931 >> 8); }

void panic_handler_c() {
    clear_graphics(VGA_RED); set_gfx_cursor(0, 0);
    draw_string("\n*** KERNEL PANIC ***\nSYSTEM HALTED.", VGA_WHITE);
    swap_buffers(); 
    while(1); 
}

void init_interrupts() {
    port_byte_out(0x20, 0x11); port_byte_out(0xA0, 0x11);
    port_byte_out(0x21, 0x20); port_byte_out(0xA1, 0x28);
    port_byte_out(0x21, 0x04); port_byte_out(0xA1, 0x02);
    port_byte_out(0x21, 0x01); port_byte_out(0xA1, 0x01);
    port_byte_out(0x21, 0xF8); port_byte_out(0xA1, 0xEF); 

    idt_reg.base = (unsigned int) &idt; idt_reg.limit = 256 * 8 - 1;
    set_idt_gate(0x00, (unsigned int)isr0_wrapper); 
    set_idt_gate(0x20, (unsigned int)irq0_wrapper); 
    set_idt_gate(0x21, (unsigned int)isr1_wrapper); 
    set_idt_gate(0x2C, (unsigned int)irq12_wrapper);
    set_idt_gate(0x80, (unsigned int)isr128_wrapper); 
    load_idt((unsigned int)&idt_reg);
}

// ==========================================
// ИДЕАЛЬНЫЙ КУРСОР WINDOWS 95 (16x16)
// 1 = Черный контур, 2 = Белая заливка, 0 = Прозрачность
// ==========================================
const unsigned char cursor_bitmap[16][16] = {
    {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,2,1,1,1,1,1,0,0,0,0,0},
    {1,2,2,1,2,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,1,2,2,1,0,0,0,0,0,0,0,0},
    {1,1,0,0,1,2,2,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,2,2,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0}
};

// ==========================================
// ТОЧКА ВХОДА MAIN
// ==========================================
void main() {
    init_interrupts();
    init_timer();
    mouse_init(); 
    fat16_init();
    init_graphics();
    
    start_shell();

    int was_gui_active = 0; 
    unsigned int bg[4] = {0, 0, 0, 0}; 
    int old_x = mouse_x; 
    int old_y = mouse_y;

    unsigned int last_tick = timer_ticks;

    while(1) {
        check_and_execute_command();  
        
        extern int gui_active;

        if (timer_ticks != last_tick) {
            last_tick = timer_ticks;

            if (gui_active) {
                was_gui_active = 1;
                
                gui_update(); 
                
                // --- РИСУЕМ КРАСИВЫЙ BITMAP-КУРСОР ---
                for (int cy = 0; cy < 16; cy++) {
                    for (int cx = 0; cx < 16; cx++) {
                        if (cursor_bitmap[cy][cx] == 1) put_pixel(mouse_x + cx, mouse_y + cy, VGA_BLACK);
                        if (cursor_bitmap[cy][cx] == 2) put_pixel(mouse_x + cx, mouse_y + cy, VGA_WHITE);
                    }
                }
                
                swap_buffers();
                
            } else {
                if (was_gui_active) {
                    clear_graphics(VGA_BLACK);
                    set_gfx_cursor(0, 0);
                    draw_string("=== OS Core Restored ===\n", VGA_LCYAN);
                    draw_string("root@os:~$ ", VGA_LGREEN);
                    swap_buffers();
                    was_gui_active = 0;
                    
                    bg[0] = get_pixel(mouse_x, mouse_y); bg[1] = get_pixel(mouse_x + 1, mouse_y);
                    bg[2] = get_pixel(mouse_x, mouse_y + 1); bg[3] = get_pixel(mouse_x + 1, mouse_y + 1);
                    old_x = mouse_x; old_y = mouse_y;
                }
                
                if (mouse_x != old_x || mouse_y != old_y) {
                    put_pixel(old_x, old_y, bg[0]); 
                    put_pixel(old_x + 1, old_y, bg[1]);
                    put_pixel(old_x, old_y + 1, bg[2]); 
                    put_pixel(old_x + 1, old_y + 1, bg[3]);
                    
                    bg[0] = get_pixel(mouse_x, mouse_y); 
                    bg[1] = get_pixel(mouse_x + 1, mouse_y);
                    bg[2] = get_pixel(mouse_x, mouse_y + 1); 
                    bg[3] = get_pixel(mouse_x + 1, mouse_y + 1);
                    
                    // В терминале оставляем маленький квадратик
                    draw_rectangle(mouse_x, mouse_y, 2, 2, VGA_WHITE);
                    old_x = mouse_x; 
                    old_y = mouse_y;
                    
                    swap_buffers(); 
                }
            }
        }
    }
}