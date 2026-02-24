#pragma once
// Читает один сектор (512 байт) по номеру LBA в указанный буфер памяти
void read_sector(unsigned int lba, unsigned char* buffer);