#pragma once

void fat16_init();
void fat16_ls();
void fat16_cat_test();
void fat16_run(char* filename);

// Добавили координаты мыши и статус клика
void fat16_draw_files(int start_x, int start_y, unsigned char text_color, int m_x, int m_y, int m_click);