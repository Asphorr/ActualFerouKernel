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
extern void isr0_wrapper();  // Паника
extern void isr1_wrapper();  // Клавиатура
extern void irq0_wrapper();  // Таймер
extern void irq12_wrapper(); // Мышь
extern void isr128_wrapper(); // Syscalls

// БЕРЕМ ТАЙМЕР ИЗ ПЛАНИРОВЩИКА (task.c)
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
                draw_rectangle(mouse_x, mouse_y, 2, 2, VGA_WHITE); // В GUI мы рисуем в буфер, это ок
                swap_buffers();
            } else {
                if (was_gui_active) {
                    clear_graphics(VGA_BLACK);
                    set_gfx_cursor(0, 0);
                    draw_string("=== OS Core Restored ===\n", VGA_LCYAN);
                    draw_string("root@os:~$ ", VGA_LGREEN);
                    swap_buffers();
                    was_gui_active = 0;
                    
                    // Рисуем стартовую мышь
                    draw_mouse_cli(mouse_x, mouse_y);
                    old_x = mouse_x; old_y = mouse_y;
                }
                
                // В терминале мы стираем и рисуем мышь напрямую в видеокарту!
                if (mouse_x != old_x || mouse_y != old_y) {
                    erase_mouse_cli(old_x, old_y);
                    draw_mouse_cli(mouse_x, mouse_y);
                    old_x = mouse_x; 
                    old_y = mouse_y;
                }
            }
        }
    }
}