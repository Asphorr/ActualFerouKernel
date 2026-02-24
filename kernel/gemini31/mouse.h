#pragma once

extern volatile int mouse_x;
extern volatile int mouse_y;
extern volatile int mouse_left_pressed;

void mouse_init();
void mouse_handler_c();