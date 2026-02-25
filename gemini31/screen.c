#include "screen.h"
#include "ports.h"
#include "util.h"

int cursor_offset = 160;

void set_cursor(int offset) {
    cursor_offset = offset;
    offset /= 2;
    port_byte_out(0x3D4, 14); port_byte_out(0x3D5, (unsigned char)(offset >> 8));
    port_byte_out(0x3D4, 15); port_byte_out(0x3D5, (unsigned char)(offset & 0xFF));
}

void scroll_screen() {
    if (cursor_offset < 80 * 25 * 2) return;
    char* vid = (char*) 0xb8000;
    memory_copy(vid + 320, vid + 160, 80 * 23 * 2);
    for (int i = 80 * 24 * 2; i < 80 * 25 * 2; i += 2) {
        vid[i] = ' '; vid[i+1] = 0x0F;
    }
    cursor_offset -= 80 * 2;
}

void draw_status_bar() {
    char* vid = (char*) 0xb8000;
    for (int i = 0; i < 160; i += 2) { vid[i] = ' '; vid[i+1] = 0x1F; }
    char* title = " Custom OS v0.4 | Clean Architecture | Timer: [  ]";
    int j = 0;
    while(title[j] != '\0') { vid[j * 2] = title[j]; vid[j * 2 + 1] = 0x1F; j++; }
}

void clear_screen() {
    char* vid = (char*) 0xb8000;
    for (int i = 160; i < 80 * 25 * 2; i += 2) { vid[i] = ' '; vid[i+1] = 0x0F; }
    set_cursor(160);
}

void print_char(char c, unsigned char color) {
    scroll_screen();
    char* vid = (char*) 0xb8000;
    if (c == '\b') {
        if (cursor_offset > 160) {
            cursor_offset -= 2; vid[cursor_offset] = ' '; vid[cursor_offset + 1] = 0x0F;
        }
    } else if (c == '\n') {
        int current_row = (cursor_offset / 2) / 80;
        cursor_offset = (current_row + 1) * 80 * 2;
    } else {
        vid[cursor_offset] = c; vid[cursor_offset + 1] = color; cursor_offset += 2;
    }
    scroll_screen();
    set_cursor(cursor_offset);
}

void print_string(char* str, unsigned char color) {
    int i = 0; while (str[i] != '\0') { print_char(str[i], color); i++; }
}