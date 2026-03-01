#include "system.h"

struct gdt_entry_struct { uint16_t limit_low; uint16_t base_low; uint8_t base_middle; uint8_t access; uint8_t granularity; uint8_t base_high; } __attribute__((packed));
struct gdt_ptr_struct { uint16_t limit; uint32_t base; } __attribute__((packed));
struct tss_entry_struct { uint32_t prev_tss, esp0, ss0, esp1, ss1, esp2, ss2; uint32_t cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi; uint32_t es, cs, ss, ds, fs, gs; uint32_t ldt, trap, iomap_base; } __attribute__((packed));

struct gdt_entry_struct gdt_entries[6]; struct gdt_ptr_struct gdt_ptr; struct tss_entry_struct tss_entry;

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low = (base & 0xFFFF); gdt_entries[num].base_middle = (base >> 16) & 0xFF; gdt_entries[num].base_high = (base >> 24) & 0xFF;
    gdt_entries[num].limit_low = (limit & 0xFFFF); gdt_entries[num].granularity = (limit >> 16) & 0x0F; gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access = access;
}

// НОВИНКА: Эта функция меняет "Ядерный Стек" при переключении задач
void set_kernel_stack(uint32_t stack) {
    tss_entry.esp0 = stack;
}

void init_gdt(void) {
    gdt_ptr.limit = (sizeof(struct gdt_entry_struct) * 6) - 1; gdt_ptr.base = (uint32_t)&gdt_entries;
    gdt_set_gate(0, 0, 0, 0, 0); 
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Ядро Код (Ring 0)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Ядро Данные (Ring 0)
    
    // ИСПРАВЛЕНИЕ: Сегменты для приложений (DPL = 3, Ring 3!)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // Пользователь Код (0x1B)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // Пользователь Данные (0x23)
    
    // TSS (Сегмент состояния задачи)
    gdt_set_gate(5, (uint32_t)&tss_entry, sizeof(tss_entry) - 1, 0x89, 0x00); 
    
    fast_fill32(&tss_entry, 0, sizeof(tss_entry)/4); 
    tss_entry.ss0 = 0x10; 
    tss_entry.esp0 = 0x0; 
    tss_entry.iomap_base = sizeof(tss_entry);
    
    __asm__ volatile (
        "lgdt %0\n\t"
        "mov $0x10, %%ax\n\t" "mov %%ax, %%ds\n\t" "mov %%ax, %%es\n\t" "mov %%ax, %%fs\n\t" "mov %%ax, %%gs\n\t" "mov %%ax, %%ss\n\t"
        "pushl $0x08\n\t" "pushl $1f\n\t" "lret\n\t"
        "1:\n\t"
        "mov $0x28, %%ax\n\t" // Загружаем индекс TSS в процессор!
        "ltr %%ax\n\t" 
        :: "m"(gdt_ptr) : "eax", "memory"
    );
}

struct idt_entry { uint16_t base_lo; uint16_t sel; uint8_t always0; uint8_t flags; uint16_t base_hi; } __attribute__((packed));
struct idt_ptr { uint16_t limit; uint32_t base; } __attribute__((packed));
struct idt_entry idt[256]; struct idt_ptr idtp;

extern void keyboard_isr_wrapper(); extern void mouse_isr_wrapper(); extern void syscall_isr_wrapper();
extern void preemptive_timer_isr(); extern void yield_isr_wrapper();

void set_idt_gate(int n, uint32_t handler, uint8_t dpl) {
    idt[n].base_lo = handler & 0xFFFF; idt[n].base_hi = (handler >> 16) & 0xFFFF;
    idt[n].sel = 0x08; idt[n].always0 = 0; idt[n].flags = 0x8E | ((dpl & 3) << 5); 
}

void fault_handler(void) { 
    extern volatile int current_task;
    if (current_task == 0) { __asm__ volatile("cli"); while(1) { __asm__ volatile("hlt"); } }
    extern void set_task_zombie(void); set_task_zombie(); 
    extern volatile int app_crashed_timer; app_crashed_timer = 200; 
    extern volatile int gui_needs_redraw; gui_needs_redraw = 1; 
    __asm__ volatile("int $0x81"); 
}

__attribute__((naked)) void isr_fault_no_err(void) { __asm__ volatile("pusha; call fault_handler; popa; iret"); }
__attribute__((naked)) void isr_fault_err(void) { __asm__ volatile("add $4, %%esp; pusha; call fault_handler; popa; iret" ::: "memory"); }

volatile uint32_t sys_ticks = 0; void timer_handler_isr(void) { sys_ticks++; }

void init_pat() {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    if (!(edx & (1 << 16))) { return; }
    uint32_t pat_lo, pat_hi;
    __asm__ volatile("rdmsr" : "=a"(pat_lo), "=d"(pat_hi) : "c"(0x277));
    pat_lo &= ~(0xFF << 8); pat_lo |= (0x01 << 8);  
    __asm__ volatile("wrmsr" :: "a"(pat_lo), "d"(pat_hi), "c"(0x277));
}

void init_sse() {
    uint32_t cr0, cr4;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0)); cr0 &= ~(1 << 2); cr0 |= (1 << 1); __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4)); cr4 |= (1 << 9) | (1 << 10); __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
    __asm__ volatile("fninit"); 
}

uint8_t read_rtc(int reg) { outb(0x70, reg); return inb(0x71); }
void get_rtc_time(int *h, int *m, int *s) { outb(0x70, 0x0A); while (inb(0x71) & 0x80); uint8_t sec = read_rtc(0x00); uint8_t min = read_rtc(0x02); uint8_t hr = read_rtc(0x04); uint8_t statusB = read_rtc(0x0B); if (!(statusB & 0x04)) { sec = (sec & 0x0F) + ((sec / 16) * 10); min = (min & 0x0F) + ((min / 16) * 10); hr = (hr & 0x0F) + (((hr & 0x70) / 16) * 10); } *s = sec; *m = min; *h = (hr + 7) % 24; }

volatile uint8_t key_ready = 0; volatile char key_buffer = 0; volatile uint8_t app_key_ready = 0; volatile char app_key_buffer = 0;
const char keymap[128] = { 0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', '\t', 'q','w','e','r','t','y','u','i','o','p','[',']', '\n', 0, 'a','s','d','f','g','h','j','k','l',';', '\'', '`', 0, '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ' };

void keyboard_handler_isr() { uint8_t sc = inb(0x60); if (sc == 0xE0) { outb(0x20, 0x20); return; } if (!(sc & 0x80)) { char c = 0; if (sc == 0x4B) { c = 17; } else if (sc == 0x4D) { c = 18; } else if (sc == 0x48) { c = 19; } else if (sc == 0x50) { c = 20; } else if (sc < 128) { c = keymap[sc]; } if (c) { key_buffer = c; key_ready = 1; app_key_buffer = c; app_key_ready = 1; } } outb(0x20, 0x20); }

volatile uint8_t mouse_moved = 1; volatile uint8_t mouse_left_pressed = 0; volatile int mouse_x = 400; volatile int mouse_y = 300; uint8_t mouse_cycle = 0; uint8_t mouse_byte[3]; 
void mouse_wait(uint8_t type) { uint32_t t = 100000; if (type == 0) { while (t--) { if (inb(0x64) & 1) return; } } else { while (t--) { if ((inb(0x64) & 2) == 0) return; } } }
void mouse_write(uint8_t data) { mouse_wait(1); outb(0x64, 0xD4); mouse_wait(1); outb(0x60, data); }
uint8_t mouse_read() { mouse_wait(0); return inb(0x60); }

void mouse_handler_isr() {
    uint8_t status = inb(0x64); if (!(status & 0x20)) { outb(0xA0, 0x20); outb(0x20, 0x20); return; } 
    uint8_t data = inb(0x60);
    switch (mouse_cycle) {
        case 0: if ((data & 0x08) == 0) break; mouse_byte[0] = data; mouse_cycle++; break;
        case 1: mouse_byte[1] = data; mouse_cycle++; break;
        case 2:
            mouse_byte[2] = data; mouse_cycle = 0; if (mouse_byte[0] & 0xC0) break; 
            mouse_left_pressed = mouse_byte[0] & 1;
            int dx = mouse_byte[1]; int dy = mouse_byte[2];
            if (mouse_byte[0] & 0x10) dx |= 0xFFFFFF00; 
            if (mouse_byte[0] & 0x20) dy |= 0xFFFFFF00; 
            mouse_x += dx; mouse_y -= dy; 
            if (mouse_x < 0) { mouse_x = 0; } if (mouse_x >= screen_w) { mouse_x = screen_w - 1; }
            if (mouse_y < 0) { mouse_y = 0; } if (mouse_y >= screen_h) { mouse_y = screen_h - 1; }
            mouse_moved = 1; break;
    } outb(0xA0, 0x20); outb(0x20, 0x20); 
}

void init_hardware() {
    init_sse(); init_pat(); 
    extern void init_paging(void); init_paging(); init_gdt(); 
    
    outb(0x20, 0x11); outb(0xA0, 0x11); outb(0x21, 0x20); outb(0xA1, 0x28); outb(0x21, 0x04); outb(0xA1, 0x02); outb(0x21, 0x01); outb(0xA1, 0x01); outb(0x43, 0x36); outb(0x40, 0x9B); outb(0x40, 0x2E); outb(0x21, 0xF8); outb(0xA1, 0xEF); 
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1; idtp.base = (uint32_t)&idt; for (int i = 0; i < 256; i++) { idt[i].base_lo = 0; idt[i].flags = 0; }
    for (int i = 0; i < 32; i++) { if (i == 8 || i == 10 || i == 11 || i == 12 || i == 13 || i == 14 || i == 17 || i == 21) { set_idt_gate(i, (uint32_t)isr_fault_err, 0); } else { set_idt_gate(i, (uint32_t)isr_fault_no_err, 0); } }
    
    set_idt_gate(32, (uint32_t)preemptive_timer_isr, 0); 
    set_idt_gate(33, (uint32_t)keyboard_isr_wrapper, 0); 
    set_idt_gate(44, (uint32_t)mouse_isr_wrapper, 0); 
    
    // ИСПРАВЛЕНИЕ: Вызов int 0x80 разрешен из Ring 3 (DPL=3)
    set_idt_gate(128, (uint32_t)syscall_isr_wrapper, 3); 
    // Внутренний Yield int 0x81 (DPL=0 - Неприступен для User Mode!)
    set_idt_gate(129, (uint32_t)yield_isr_wrapper, 0); 
    
    __asm__ volatile("lidt %0" : : "m" (idtp));
    mouse_wait(1); outb(0x64, 0xA8); mouse_wait(1); outb(0x64, 0x20); mouse_wait(0); uint8_t s = (inb(0x60) | 2); mouse_wait(1); outb(0x64, 0x60); mouse_wait(1); outb(0x60, s); mouse_write(0xF6); mouse_read(); mouse_write(0xF4); mouse_read(); 
}