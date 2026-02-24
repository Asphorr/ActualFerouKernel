#include "mouse.h"
#include "ports.h"
#include "graphics.h"

volatile int mouse_x = 160;
volatile int mouse_y = 100;
volatile int mouse_left_pressed = 0;

int old_mouse_x = 160;
int old_mouse_y = 100;
unsigned char mouse_bg[4] = {0,0,0,0};
int mouse_drawn = 0;

unsigned char mouse_cycle = 0;
char mouse_byte[3];

void mouse_wait(unsigned char a_type) {
    unsigned int time_out = 100000;
    if (a_type == 0) { while (time_out--) if ((port_byte_in(0x64) & 1) == 1) return; } 
    else { while (time_out--) if ((port_byte_in(0x64) & 2) == 0) return; }
}

void mouse_write(unsigned char write) {
    mouse_wait(1); port_byte_out(0x64, 0xD4);
    mouse_wait(1); port_byte_out(0x60, write);
}
unsigned char mouse_read() { mouse_wait(0); return port_byte_in(0x60); }

void mouse_init() {
    mouse_wait(1); port_byte_out(0x64, 0xA8);
    mouse_wait(1); port_byte_out(0x64, 0x20);
    mouse_wait(0); unsigned char status = (port_byte_in(0x60) | 2);
    mouse_wait(1); port_byte_out(0x64, 0x60);
    mouse_wait(1); port_byte_out(0x60, status);
    mouse_write(0xF6); mouse_read();
    mouse_write(0xF4); mouse_read();
}

void mouse_hide() {
    if (mouse_drawn) {
        put_pixel(old_mouse_x, old_mouse_y, mouse_bg[0]);
        put_pixel(old_mouse_x + 1, old_mouse_y, mouse_bg[1]);
        put_pixel(old_mouse_x, old_mouse_y + 1, mouse_bg[2]);
        put_pixel(old_mouse_x + 1, old_mouse_y + 1, mouse_bg[3]);
        mouse_drawn = 0;
    }
}

void mouse_show() {
    if (!mouse_drawn) {
        mouse_bg[0] = get_pixel(mouse_x, mouse_y);
        mouse_bg[1] = get_pixel(mouse_x + 1, mouse_y);
        mouse_bg[2] = get_pixel(mouse_x, mouse_y + 1);
        mouse_bg[3] = get_pixel(mouse_x + 1, mouse_y + 1);

        draw_rectangle(mouse_x, mouse_y, 2, 2, VGA_WHITE); 
        old_mouse_x = mouse_x;
        old_mouse_y = mouse_y;
        mouse_drawn = 1;
    }
}

// Главная функция обновления, вызывается в ядре
void mouse_update() {
    if (mouse_x != old_mouse_x || mouse_y != old_mouse_y) {
        mouse_hide();
        mouse_show();
        
        // ВАЖНО: Если мы не в GUI-режиме, обновляем экран сразу после отрисовки мыши
        extern int gui_active;
        if (!gui_active) {
            swap_buffers(); 
        }
    }
}

void mouse_handler_c() {
    unsigned char status = port_byte_in(0x64);
    if (!(status & 0x20)) { port_byte_out(0xA0, 0x20); port_byte_out(0x20, 0x20); return; }

    unsigned char b = port_byte_in(0x60);
    if (mouse_cycle == 0 && (b & 0x08) == 0) { port_byte_out(0xA0, 0x20); port_byte_out(0x20, 0x20); return; }

    mouse_byte[mouse_cycle++] = b;

    if (mouse_cycle == 3) {
        mouse_cycle = 0;
        int d_x = mouse_byte[1]; int d_y = mouse_byte[2];

        if (mouse_byte[0] & 0x10) d_x |= 0xFFFFFF00;
        if (mouse_byte[0] & 0x20) d_y |= 0xFFFFFF00;

        mouse_x += d_x; mouse_y -= d_y;

        if (mouse_x < 0) mouse_x = 0; if (mouse_x > 798) mouse_x = 798;
        if (mouse_y < 0) mouse_y = 0; if (mouse_y > 598) mouse_y = 598;

        mouse_left_pressed = mouse_byte[0] & 0x01;
    }
    port_byte_out(0xA0, 0x20); port_byte_out(0x20, 0x20);
}