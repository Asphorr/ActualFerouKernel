#include "disk.h"
#include "ports.h"

void ata_wait_bsy() {
    while(port_byte_in(0x1F7) & 0x80);
}

void ata_wait_drq() {
    while(!(port_byte_in(0x1F7) & 0x08));
}

void read_sector(unsigned int lba, unsigned char* buffer) {
    ata_wait_bsy(); 

    port_byte_out(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); 
    port_byte_out(0x1F2, 1);                           
    port_byte_out(0x1F3, (unsigned char) lba);         
    port_byte_out(0x1F4, (unsigned char)(lba >> 8));   
    port_byte_out(0x1F5, (unsigned char)(lba >> 16));  
    port_byte_out(0x1F7, 0x20); // Команда READ

    // ОФИЦИАЛЬНАЯ ПАУЗА 400 НАНОСЕКУНД (ATA SPECIFICATION)
    port_byte_in(0x1F7);
    port_byte_in(0x1F7);
    port_byte_in(0x1F7);
    port_byte_in(0x1F7);

    ata_wait_bsy(); 
    ata_wait_drq(); 

    for (int i = 0; i < 256; i++) {
        unsigned short word = port_word_in(0x1F0); 
        buffer[i * 2] = (unsigned char)(word & 0xFF);         
        buffer[i * 2 + 1] = (unsigned char)((word >> 8) & 0xFF); 
    }
}