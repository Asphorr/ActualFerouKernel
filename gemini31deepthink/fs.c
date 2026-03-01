#include "system.h"

// Глобальный кэш файловой системы (ускоряет чтение в разы)
static uint32_t fat_rstart = 0;
static uint32_t fat_dstart = 0;
static uint16_t fat_bps = 0;
static uint32_t fat_rsects = 0;
static uint8_t fat_spc = 0;
static int fs_initialized = 0;

void ata_read_sector(uint32_t lba, uint8_t *buffer) {
    while ((inb(0x1F7) & 0x80) != 0); 
    
    outb(0x1F6, 0xF0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1); 
    outb(0x1F3, (uint8_t)lba); 
    outb(0x1F4, (uint8_t)(lba >> 8)); 
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20); 
    
    while ((inb(0x1F7) & 0x08) == 0); 
    
    for (int i = 0; i < 256; i++) { 
        uint16_t word = inw(0x1F0); 
        buffer[i * 2] = (uint8_t)word; 
        buffer[i * 2 + 1] = (uint8_t)(word >> 8); 
    }
}

void ata_write_sector(uint32_t lba, uint8_t *buffer) {
    uint32_t timeout = 100000;
    while ((inb(0x1F7) & 0x80) != 0 && --timeout); 
    
    outb(0x1F6, 0xF0 | ((lba >> 24) & 0x0F)); 
    outb(0x1F2, 1); 
    outb(0x1F3, (uint8_t)lba); 
    outb(0x1F4, (uint8_t)(lba >> 8)); 
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30); 
    
    timeout = 100000;
    while ((inb(0x1F7) & 0x08) == 0 && --timeout); 
    
    for (int i = 0; i < 256; i++) { 
        uint16_t word = buffer[i * 2] | (buffer[i * 2 + 1] << 8); 
        outw(0x1F0, word); 
    }
    
    timeout = 100000;
    while ((inb(0x1F7) & 0x80) != 0 && --timeout); 
}

void fs_init(void) {
    uint8_t boot[512]; 
    ata_read_sector(0, boot); 
    
    fat_bps = *(uint16_t*)(boot + 11); 
    if (fat_bps != 512) {
        return; 
    }
    
    fat_spc = boot[13]; 
    uint16_t rsvd = *(uint16_t*)(boot + 14); 
    uint8_t nfats = boot[16];
    uint16_t rent = *(uint16_t*)(boot + 17); 
    uint16_t spf = *(uint16_t*)(boot + 22);
    
    fat_rstart = rsvd + (nfats * spf); 
    fat_rsects = (rent * 32 + 511) / 512;
    fat_dstart = fat_rstart + fat_rsects;
    fs_initialized = 1;
}

void* load_fat16_file(const char* target_name, uint32_t* out_size) {
    if (!fs_initialized) {
        return NULL;
    }
    
    uint8_t sec[512]; 
    uint32_t clus = 0, sz = 0;
    
    for (uint32_t s = 0; s < fat_rsects; s++) {
        ata_read_sector(fat_rstart + s, sec);
        for (int i = 0; i < 512; i += 32) {
            if (sec[i] == 0x00) break; 
            if (sec[i] == 0xE5 || sec[i + 11] == 0x0F) continue; 
            
            int match = 1; 
            for (int j = 0; j < 11; j++) {
                if (sec[i + j] != target_name[j]) {
                    match = 0;
                }
            }
            if (match) { 
                clus = *(uint16_t*)(sec + i + 26); 
                sz = *(uint32_t*)(sec + i + 28); 
                break; 
            }
        }
        if (clus) break;
    }
    
    if (!clus) return NULL;
    
    if (out_size) {
        *out_size = sz;
    }
    
    void* file_buf = kmalloc(sz + fat_bps); 
    for (uint32_t r = 0; r < (sz + fat_bps - 1) / fat_bps; r++) {
        ata_read_sector(fat_dstart + (clus - 2) * fat_spc + r, (uint8_t*)file_buf + r * fat_bps);
    }
    return file_buf;
}

void fs_write_file(const char* target_name, uint8_t* data, uint32_t size) {
    if (!fs_initialized) return;
    
    uint8_t sec[512]; 
    uint32_t clus = 0, target_sector = 0;
    
    for (uint32_t s = 0; s < fat_rsects; s++) {
        ata_read_sector(fat_rstart + s, sec);
        for (int i = 0; i < 512; i += 32) {
            if (sec[i] == 0x00) break; 
            
            int match = 1; 
            for (int j = 0; j < 11; j++) {
                if (sec[i + j] != target_name[j]) match = 0;
            }
            if (match) { 
                clus = *(uint16_t*)(sec + i + 26); 
                *(uint32_t*)(sec + i + 28) = size; 
                target_sector = fat_rstart + s;
                ata_write_sector(target_sector, sec); 
                break; 
            }
        }
        if (clus) break;
    }
    
    if (clus == 0) return;
    
    uint32_t swrite = (size + fat_bps - 1) / fat_bps; 
    if (swrite == 0) swrite = 1;
    uint32_t cursec = fat_dstart + (clus - 2) * fat_spc;
    
    for (uint32_t r = 0; r < swrite; r++) {
        uint8_t pad[512] = {0};
        uint32_t chunk = (size > 512) ? 512 : size;
        for (uint32_t b = 0; b < chunk; b++) {
            pad[b] = data[r * 512 + b];
        }
        ata_write_sector(cursec + r, pad); 
        if (size >= chunk) {
            size -= chunk;
        }
    }
}

void fs_get_files(char* out_buf) {
    if (!fs_initialized) return;
    
    uint8_t sec[512]; 
    int pos = 0;
    
    for (uint32_t s = 0; s < fat_rsects; s++) {
        ata_read_sector(fat_rstart + s, sec);
        for (int i = 0; i < 512; i += 32) {
            if (sec[i] == 0) goto done; 
            if (sec[i] == 0xE5 || sec[i+11] == 0x0F) continue; 
            
            for (int j = 0; j < 11; j++) {
                out_buf[pos++] = sec[i+j];
            }
            out_buf[pos++] = '\n';
        }
    }
done: 
    out_buf[pos] = '\0';
}