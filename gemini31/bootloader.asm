[org 0x7c00]        
[bits 16]           

xor ax, ax          
mov ds, ax          
mov es, ax          
mov ss, ax          
mov bp, 0x8000      
mov sp, bp          

mov [BOOT_DRIVE], dl

mov bx, MSG_LOAD
call print_string

; Расширенное чтение (LBA) INT 13h AH=42h
mov ah, 0x42
mov dl, [BOOT_DRIVE]
mov si, LBA_PACKET
int 0x13
jc disk_error       

jmp 0x1000

disk_error:
    mov bx, MSG_ERR
    call print_string
    jmp $           

print_string:
    pusha           
    mov ah, 0x0e    
.loop:
    mov al, [bx]    
    cmp al, 0       
    je .done
    int 0x10        
    inc bx          
    jmp .loop
.done:
    popa            
    ret

BOOT_DRIVE      db 0
MSG_LOAD        db 'Loading Kernel from Hidden Sectors...', 13, 10, 0
MSG_ERR         db 'Disk read error!', 13, 10, 0

align 4
LBA_PACKET:
    db 0x10         ; Размер пакета (16 байт)
    db 0            ; Зарезервировано (0)
    dw 50          ; Читаем 100 секторов (50 КБ ядра - с огромным запасом!)
    dw 0x1000       ; Смещение буфера (Куда грузить)
    dw 0x0000       ; Сегмент буфера
    dq 1            ; НАЧАЛЬНЫЙ СЕКТОР (LBA 1 - сразу после загрузчика)

; Заполнение нулями и пустая таблица MBR (ее заполнит parted)
times 446-($-$$) db 0
times 64 db 0
dw 0xaa55