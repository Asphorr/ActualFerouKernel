#include "shell.h"
#include "ports.h"
#include "graphics.h"
#include "util.h"
#include "memory.h"
#include "disk.h"
#include "gui.h"
#include "mouse.h"
#include "fat16.h"

extern unsigned int timer_ticks; 

char cmd_buffer[256];
int cmd_length = 0;
int shift_pressed = 0;
int command_ready = 0; 

void execute_command(char* command) {
    if (cmd_length == 0) return;
    
    if (str_compare(command, "help")) {
        draw_string("Commands: help, clear, reboot, uptime, memtest, disktest, gui, ls, cat, run app.bin\n", VGA_WHITE);
    } else if (str_compare(command, "clear")) {
        clear_graphics(VGA_BLACK); set_gfx_cursor(0, 0); 
    } else if (str_compare(command, "reboot")) {
        port_byte_out(0x64, 0xFE);
    } else if (str_compare(command, "uptime")) {
        char time_str[16]; int_to_string(timer_ticks / 100, time_str); 
        draw_string("OS Uptime: ", VGA_LCYAN); draw_string(time_str, VGA_YELLOW); draw_string(" sec\n", VGA_LCYAN);
    } else if (str_compare(command, "disktest")) {
        draw_string("Reading HDD...\n", VGA_LCYAN); 
        unsigned char* sector_buffer = (unsigned char*) kmalloc(512);
        read_sector(0, sector_buffer);
        if (sector_buffer[510] == 0x55 && sector_buffer[511] == 0xAA) draw_string("SUCCESS!\n", VGA_LGREEN);
        else draw_string("ERROR!\n", VGA_LRED); 
    } else if (str_compare(command, "memtest")) {
        unsigned int phys_addr = kmalloc(100);
        char hex_str[16]; int_to_hex(phys_addr, hex_str);
        draw_string("Allocated 100 bytes at: ", VGA_LCYAN); draw_string(hex_str, VGA_YELLOW); draw_string("\n", VGA_WHITE);
    } else if (str_compare(command, "ls")) {
        fat16_ls();
    } else if (str_starts_with(command, "cat")) {
        fat16_cat_test();
    } else if (str_compare(command, "run app.bin")) {
        fat16_run("APP     BIN"); 
        clear_graphics(VGA_BLACK);
        set_gfx_cursor(0, 0);
        draw_string("=== OS Core Restored ===\n", VGA_LCYAN);
    } else if (str_compare(command, "gui")) {
        gui_start_desktop(); 
    } else {
        draw_string("Unknown command\n", VGA_LRED); 
    }

    swap_buffers(); 
}

void check_and_execute_command() {
    if (command_ready) {
        execute_command(cmd_buffer);
        cmd_length = 0;
        command_ready = 0;
        extern int gui_active;
        if (!gui_active) {
            draw_string("root@os:~$ ", VGA_LGREEN); 
            swap_buffers(); 
            draw_mouse_cli(mouse_x, mouse_y); // Возвращаем мышь поверх текста
        }
    }
}

const char scancode_ascii[] = {
    0, 0, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    0, 'q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ' 
};

const char scancode_ascii_shift[] = {
    0, 0, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    0, 'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0, 'A','S','D','F','G','H','J','K','L',':','"','~',
    0, '|','Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' ' 
};

void keyboard_handler_c() {
    unsigned char scancode = port_byte_in(0x60);
    
    extern int gui_active;
    if (gui_active) { port_byte_out(0x20, 0x20); return; }

    if (scancode == 0x2A || scancode == 0x36) shift_pressed = 1; 
    if (scancode == 0xAA || scancode == 0xB6) shift_pressed = 0; 

    if (!(scancode & 0x80)) { 
        if (scancode <= 0x39) {
            char ascii = shift_pressed ? scancode_ascii_shift[scancode] : scancode_ascii[scancode];
            if (ascii == '\b') {
                if (cmd_length > 0) { cmd_length--; char bs[2] = {'\b', '\0'}; draw_string(bs, VGA_WHITE); }
            } else if (ascii == '\n') {
                draw_string("\n", VGA_WHITE);
                cmd_buffer[cmd_length] = '\0'; 
                command_ready = 1; 
            } else if (ascii != 0) {
                if (cmd_length < 255) {
                    cmd_buffer[cmd_length] = ascii; cmd_length++;
                    char str[2] = {ascii, '\0'}; draw_string(str, VGA_WHITE); 
                }
            }
        }
    }
    
    swap_buffers();
    draw_mouse_cli(mouse_x, mouse_y); // Перерисовываем мышь поверх напечатанной буквы
    port_byte_out(0x20, 0x20); 
}

void start_shell() {
    clear_graphics(VGA_BLACK);
    draw_string("=== Modular OS Core v1.0 ===\n", VGA_LCYAN);
    draw_string("Type 'help' to begin.\n\n", VGA_WHITE);
    draw_string("root@os:~$ ", VGA_LGREEN);
    swap_buffers(); 
    draw_mouse_cli(mouse_x, mouse_y);
}