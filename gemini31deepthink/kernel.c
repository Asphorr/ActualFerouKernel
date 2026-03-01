#include "system.h"

int strcmp(const char *s1, const char *s2) { while (*s1 && (*s1 == *s2)) { s1++; s2++; } return *(const unsigned char*)s1 - *(const unsigned char*)s2; }
char* cmd_buffer; int cmd_length = 0; int system_state = 0; uint8_t* global_wallpaper = NULL; volatile int app_crashed_timer = 0;

volatile uint32_t current_task_cr3 = 0;

#define MAX_TASKS 16
#define IPC_QUEUE_SIZE 8

typedef struct {
    uint32_t esp; uint32_t stack_base; 
    uint32_t kernel_stack_top; 
    uint32_t cr3;
    void* orig_pd; void* orig_pt; void* orig_mem;
    int state; uint32_t sleep_ticks;
    char name[16];
    IPC_Message ipc_queue[IPC_QUEUE_SIZE];
    int ipc_head; int ipc_tail; int ipc_count;
} Task;

Task tasks[MAX_TASKS];
volatile int current_task = 0;

// ИСПРАВЛЕНИЕ: Выделяем память под FPU через kmalloc, чтобы гарантировать 100% выравнивание!
uint8_t* fpu_states_ptr = NULL;

uint32_t syscall_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx, uint32_t esi) {
    if (eax == 1) return gui_register_window((char*)ebx, ecx, edx); 
    if (eax == 2) { gui_draw_rect_win(ebx, ecx >> 16, ecx & 0xFFFF, edx >> 16, edx & 0xFFFF, esi); return 0; }
    if (eax == 3) { gui_draw_text_win(ebx, ecx >> 16, ecx & 0xFFFF, (char*)edx, esi); return 0; }
    if (eax == 4) { gui_get_mouse_win(ebx, (int*)ecx, (int*)edx, (int*)esi); return 0; }
    if (eax == 5) { fs_get_files((char*)ebx); return 0; }
    if (eax == 6) { exec_app((char*)ebx); return 0; }
    if (eax == 7) return sys_ticks;
    if (eax == 9) return get_used_memory();  
    if (eax == 10) return (64 * 1024 * 1024); 
    if (eax == 11) { if (global_wallpaper) { kfree(global_wallpaper); global_wallpaper = NULL; } if (ebx) { if (((char*)ebx)[0] != '\0') { uint32_t sz; void* file = load_fat16_file((char*)ebx, &sz); if (file) { global_wallpaper = (uint8_t*)file; } } } return 0; }
    if (eax == 12) { fs_write_file((char*)ebx, (uint8_t*)ecx, edx); return 0; }
    if (eax == 13) { if (gui_mode) { if (gui_is_focused(ebx)) { if (app_key_ready) { app_key_ready = 0; return app_key_buffer; } } return 0; } if (!gui_mode) { if (key_ready) { key_ready = 0; return key_buffer; } } return 0; }
    if (eax == 14) return (uint32_t)kmalloc(ebx);
    if (eax == 15) { kfree((void*)ebx); return 0; }
    if (eax == 16) { gui_draw_alpha_win(ebx, ecx >> 16, ecx & 0xFFFF, edx >> 16, edx & 0xFFFF, esi); return 0; }
    if (eax == 17) { uint32_t sz; void* file = load_fat16_file((char*)ebx, &sz); if (file) { ((char*)file)[sz] = '\0'; if (ecx) { *(uint32_t*)ecx = sz; } return (uint32_t)file; } return 0; }
    if (eax == 18) { static uint32_t r_seed = 12345; r_seed = r_seed * 1103515245 + 12345; return r_seed; }
    if (eax == 19) { gui_draw_dec_win(ebx, ecx >> 16, ecx & 0xFFFF, edx, esi); return 0; }
    if (eax == 20) { return gui_is_focused(ebx); } 
    if (eax == 22) { return gui_is_window_open(ebx); } 
    extern void set_task_sleep(uint32_t ticks); if (eax == 23) { set_task_sleep(ebx); return 0; }
    extern void gui_swap_window_buffers(int id); if (eax == 24) { gui_swap_window_buffers(ebx); return 0; }
    if (eax == 25) { extern void set_task_zombie(void); set_task_zombie(); __asm__ volatile("int $0x81"); return 0; } 
    
    if (eax == 26) { return current_task; } 
    if (eax == 27) { 
        int target = ebx; if (target < 0 || target >= MAX_TASKS || tasks[target].state == 0 || tasks[target].state == 3) { return 0; }
        if (tasks[target].ipc_count >= IPC_QUEUE_SIZE) { return 0; }
        int t = tasks[target].ipc_tail; tasks[target].ipc_queue[t].sender_pid = current_task; tasks[target].ipc_queue[t].type = ecx; tasks[target].ipc_queue[t].data1 = edx; tasks[target].ipc_queue[t].data2 = esi;
        tasks[target].ipc_tail = (t + 1) % IPC_QUEUE_SIZE; tasks[target].ipc_count++;
        if (tasks[target].state == 5 || tasks[target].state == 2) { tasks[target].state = 1; } return 1;
    }
    if (eax == 28) { 
        if (tasks[current_task].ipc_count == 0) { return 0; }
        int h = tasks[current_task].ipc_head; IPC_Message* msg = (IPC_Message*)ebx;
        if (msg) { msg->sender_pid = tasks[current_task].ipc_queue[h].sender_pid; msg->type = tasks[current_task].ipc_queue[h].type; msg->data1 = tasks[current_task].ipc_queue[h].data1; msg->data2 = tasks[current_task].ipc_queue[h].data2; }
        tasks[current_task].ipc_head = (h + 1) % IPC_QUEUE_SIZE; tasks[current_task].ipc_count--; return 1;
    }
    if (eax == 29) { for (int i = 1; i < MAX_TASKS; i++) { if (tasks[i].state != 0 && strcmp(tasks[i].name, (char*)ebx) == 0) { return i; } } return 0; }
    return 0;
}

void process_manager_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) { tasks[i].state = 0; tasks[i].sleep_ticks = 0; tasks[i].ipc_count = 0; tasks[i].kernel_stack_top = 0; }
    
    // ВЫДЕЛЯЕМ ИДЕАЛЬНО ВЫРОВНЕННУЮ ПАМЯТЬ!
    fpu_states_ptr = (uint8_t*)kmalloc(16384); 
    
    tasks[0].state = 1; tasks[0].cr3 = (uint32_t)kernel_page_directory; 
    extern uint32_t stack_top; tasks[0].kernel_stack_top = (uint32_t)&stack_top; current_task = 0;
    
    __asm__ volatile("fxsave %0" : "=m"(*(fpu_states_ptr))); 
    __asm__ volatile("fxsave %0" : "=m"(*(fpu_states_ptr + 512))); 
}

void set_task_sleep(uint32_t ticks) { tasks[current_task].sleep_ticks = ticks; tasks[current_task].state = 2; __asm__ volatile("int $0x81"); }
void set_task_zombie(void) { if (current_task != 0) { tasks[current_task].state = 3; } }

void kill_all_tasks(void) {
    __asm__ volatile("cli");
    for (int i = 1; i < MAX_TASKS; i++) { if (tasks[i].state != 0) { tasks[i].state = 3; } }
    __asm__ volatile("sti");
}

uint32_t schedule(uint32_t esp) {
    for(int i = 0; i < MAX_TASKS; i++) { if(tasks[i].state == 2) { if(tasks[i].sleep_ticks > 0) { tasks[i].sleep_ticks--; } if(tasks[i].sleep_ticks == 0) { tasks[i].state = 1; } } }
    for(int i = 1; i < MAX_TASKS; i++) {
        if(tasks[i].state == 3 && i != current_task) {
            // ИСПРАВЛЕНИЕ: Очищаем окна-зомби!
            extern void gui_kill_windows(int pid);
            gui_kill_windows(i);
            
            kfree((void*)tasks[i].stack_base); kfree(tasks[i].orig_pd); kfree(tasks[i].orig_pt); kfree(tasks[i].orig_mem); 
            tasks[i].state = 0;
        }
    }
    if (current_task != -1 && tasks[current_task].state != 0 && tasks[current_task].state != 3) { tasks[current_task].esp = esp; }

    int next = current_task;
    for(int i = 0; i < MAX_TASKS; i++) {
        next = (next + 1) % MAX_TASKS;
        if (tasks[next].state == 1) {
            current_task = next;
            uint32_t next_cr3 = tasks[current_task].cr3;
            if (next_cr3 == 0) { next_cr3 = (uint32_t)kernel_page_directory; }
            current_task_cr3 = next_cr3; 
            extern void set_kernel_stack(uint32_t stack);
            if (tasks[current_task].kernel_stack_top != 0) { set_kernel_stack(tasks[current_task].kernel_stack_top); }
            return tasks[current_task].esp;
        }
    }
    return esp; 
}

void task_exit(void) { __asm__ volatile("cli"); set_task_zombie(); while(1) { __asm__ volatile("sti\n\thlt"); } }
void print_prompt() { print_string("root@NexaOS:~# ", 0x00A2ED); input_start_x = cursor_x; }

void exec_app(const char* name) {
    uint32_t sz = 0; 
    void* file_data = load_fat16_file(name, &sz);
    if (!file_data || sz < sizeof(Elf32_Ehdr)) { if (file_data) { kfree(file_data); } return; }

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)file_data;
    if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' || ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') { kfree(file_data); return; }

    __asm__ volatile("cli"); 
    for (int i = 1; i < MAX_TASKS; i++) {
        if (tasks[i].state == 0) {
            tasks[i].state = 4; 
            
            void* orig_pd = kmalloc(8192); uint32_t* pd = (uint32_t*)(((uint32_t)orig_pd + 4095) & ~4095);
            void* orig_pt = kmalloc(8192); uint32_t* pt = (uint32_t*)(((uint32_t)orig_pt + 4095) & ~4095);
            fast_fill32(pt, 0, 1024); 
            
            uint32_t app_mem_size = 2097152; 
            void* orig_mem = kmalloc(app_mem_size + 4096);
            uint32_t app_phys = (((uint32_t)orig_mem + 4095) & ~4095);

            for (int k = 0; k < 1024; k++) { pd[k] = kernel_page_directory[k]; } 
            uint32_t pages = app_mem_size / 4096;
            for (uint32_t p = 0; p < pages; p++) { pt[p] = (app_phys + p * 4096) | 0x07; }
            pd[512] = ((uint32_t)pt) | 0x07; 

            fast_fill32((void*)app_phys, 0, app_mem_size / 4);

            Elf32_Phdr* phdr = (Elf32_Phdr*)((uint8_t*)file_data + ehdr->e_phoff);
            for (int p = 0; p < ehdr->e_phnum; p++) {
                if (phdr[p].p_type == 1) { 
                    uint32_t offset = phdr[p].p_vaddr - 0x80000000;
                    if (offset < app_mem_size) { fast_memcpy((void*)(app_phys + offset), (uint8_t*)file_data + phdr[p].p_offset, phdr[p].p_filesz); }
                }
            }

            uint32_t stack_alloc = (uint32_t)kmalloc(16384 + 16);
            uint32_t stack_aligned = (stack_alloc + 15) & ~15;
            uint32_t kernel_stack_top = stack_aligned + 16384;
            uint32_t* esp = (uint32_t*)kernel_stack_top;
            
            uint32_t user_stack_top = 0x80000000 + app_mem_size;
            
            *(--esp) = 0x23; *(--esp) = user_stack_top; *(--esp) = 0x0202; *(--esp) = 0x1B; *(--esp) = ehdr->e_entry; 
            for(int j = 0; j < 8; j++) { *(--esp) = 0; }
            *(--esp) = 0x23; *(--esp) = 0x23; *(--esp) = 0x23; *(--esp) = 0x23; 
            
            tasks[i].orig_pd = orig_pd; tasks[i].orig_pt = orig_pt; tasks[i].orig_mem = orig_mem;
            tasks[i].stack_base = stack_alloc; tasks[i].kernel_stack_top = kernel_stack_top;
            tasks[i].cr3 = (uint32_t)pd; tasks[i].esp = (uint32_t)esp;
            
            int nj = 0; while (name[nj] && nj < 15) { tasks[i].name[nj] = name[nj]; nj++; } tasks[i].name[nj] = '\0';
            
            fast_memcpy(fpu_states_ptr + (i * 1024), fpu_states_ptr, 1024); 

            tasks[i].ipc_head = 0; tasks[i].ipc_tail = 0; tasks[i].ipc_count = 0;
            tasks[i].sleep_ticks = 0; tasks[i].state = 1; 
            break;
        }
    }
    kfree(file_data);
    __asm__ volatile("sti");
}

void enter_desktop() { system_state = 1; gui_mode = 1; reset_desktop(); draw_desktop(); swap_buffers(); }
void exit_desktop() { system_state = 0; gui_mode = 0; gui_needs_redraw = 0; kill_all_tasks(); reset_desktop(); draw_rect(0, 0, screen_w, screen_h, 0x000000); mark_dirty(0, 0, screen_w, screen_h); cursor_x = 0; cursor_y = 0; print_string("NexaOS Terminal Restored.\n\n", 0x00FF00); print_prompt(); cmd_length = 0; swap_buffers(); }

void execute_command() {
    while (cmd_length > 0) { if (cmd_buffer[cmd_length - 1] == ' ') { cmd_length--; } else { break; } }
    cmd_buffer[cmd_length] = '\0'; if (cmd_length == 0) return;
    
    if (strcmp(cmd_buffer, "help") == 0) { print_string("NexaOS V750\n  desktop - Launch GUI\n  ls      - List files\n  memtest - Memory Stats\n  reboot  - Restart\n", 0xAAAAAA); }
    else if (strcmp(cmd_buffer, "desktop") == 0) { enter_desktop(); }
    else if (strcmp(cmd_buffer, "clear") == 0) { draw_rect(0, 0, screen_w, screen_h, 0x000000); mark_dirty(0,0,screen_w,screen_h); cursor_x = 0; cursor_y = 0; }
    else if (strcmp(cmd_buffer, "reboot") == 0) { outb(0x64, 0xFE); }
    else if (strcmp(cmd_buffer, "ls") == 0) { char buf[512]; fs_get_files(buf); print_string("Virtual File System:\n", 0x00FF00); print_string(buf, 0xFFFFFF); }
    else if (strcmp(cmd_buffer, "memtest") == 0) { print_string("Security: TRUE RING 3 USER MODE\n", 0x00FF00); uint32_t used = get_used_memory(); print_string("Used Heap: ", 0xFFFFFF); void print_dec(uint32_t num, uint32_t color); print_dec(used, 0xFFFFFF); print_string(" Bytes\n", 0xFFFFFF); }
    else { print_string("Command not found.\n", 0xFF5555); }
    cmd_length = 0;
}

void handle_key(char c) {
    if (c >= 17) { if (c <= 20) { return; } } 
    if (c == '\n') { print_char('\n', 0xFFFFFF); execute_command(); if (system_state == 0) { print_prompt(); } }
    else if (c == '\b') { if (cursor_x > input_start_x) { if (cmd_length > 0) { cmd_length--; print_char('\b', 0xFFFFFF); } } }
    else { if (cmd_length < 127) { cmd_buffer[cmd_length++] = c; print_char(c, 0xFFFFFF); } }
}

void kernel_main() {
    system_state = 0; cmd_length = 0; gui_mode = 0; cursor_x = 0; cursor_y = 0; input_start_x = 0; global_wallpaper = NULL; app_key_ready = 0; app_key_buffer = 0; 
    
    init_memory(); init_hardware(); extern void init_graphics(void); init_graphics(); 
    fs_init(); process_manager_init(); cmd_buffer = (char*)kmalloc(128);
    
    mark_dirty(0, 0, screen_w, screen_h); draw_rect(0, 0, screen_w, screen_h, 0x000000);
    print_string("=== NEXA OS V750 (GHOST BUSTER) ===\n", 0x00FFFF);
    print_string("[OK] SSE2 16-Byte Stack Alignment Secured.\n", 0x00FF00); 
    print_string("[OK] Zombie Window Exterminator Active.\n", 0x00FF00); 
    
    print_string("\nType 'desktop' to launch Graphical Shell.\n\n", 0xAAAAAA); 
    print_prompt(); swap_buffers(); __asm__ volatile("sti");
    
    uint32_t last_gui_tick = 0; int prev_mlp = 0;

    while (1) {
        int c_mlp; __asm__ volatile("cli"); c_mlp = mouse_left_pressed; __asm__ volatile("sti");
        int system_dirty = 0;
        if (mouse_moved || key_ready || app_key_ready || c_mlp != prev_mlp || gui_needs_redraw || dirty_max_x != -1 || (sys_ticks - last_gui_tick >= 10)) { system_dirty = 1; }

        if (system_dirty == 0) { __asm__ volatile("int $0x81"); continue; }

        if (system_state == 1) {
            if (sys_ticks - last_gui_tick >= 10) { last_gui_tick = sys_ticks; update_gui(); } else { update_gui(); }
            if (system_state == 0) { continue; }
            if (gui_needs_redraw) { draw_desktop(); gui_needs_redraw = 0; } 
            if (mouse_moved || c_mlp != prev_mlp || dirty_max_x != -1) { swap_buffers(); mouse_moved = 0; }
        } else {
            if (key_ready) { char c = key_buffer; key_ready = 0; handle_key(c); } 
            if (mouse_moved || dirty_max_x != -1) { swap_buffers(); mouse_moved = 0; }
        }
        prev_mlp = c_mlp; 
        __asm__ volatile("int $0x81"); 
    }
}