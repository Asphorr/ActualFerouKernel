[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drive], dl

    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    jc disk_error

    mov ax, 0x4F01
    mov cx, 0x118       ; Запрашиваем информацию о HD-режиме 1024x768
    mov di, 0x5000      
    int 0x10

    mov ax, 0x4F02
    mov bx, 0x4118      ; ВКЛЮЧАЕМ 1024x768x32!
    int 0x10

    cli
    in al, 0x92
    or al, 2
    out 0x92, al
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:init_pm

disk_error:
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    cli
.hang:
    hlt
    jmp .hang

align 4
dap:
    db 0x10, 0
    dw 100              
    dw 0x0000, 0x1000   
    dq 1                

gdt_start:
gdt_null: dq 0x0
gdt_code: dw 0xFFFF, 0x0
          db 0x0, 10011010b, 11001111b, 0x0
gdt_data: dw 0xFFFF, 0x0
          db 0x0, 10010010b, 11001111b, 0x0
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

[BITS 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebp, 0x90000
    mov esp, ebp
    jmp CODE_SEG:0x10000

boot_drive db 0
times 510 - ($ - $$) db 0
dw 0xAA55