MBALIGN  equ  1 << 0
MEMINFO  equ  1 << 1
VIDINFO  equ  1 << 2
FLAGS    equ  MBALIGN | MEMINFO | VIDINFO
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    dd 0,0,0,0,0
    dd 0
    dd 1024
    dd 768
    dd 32

section .text
global _start
extern kernel_main

_start:
    cli
    mov esp, stack_top
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang

global preemptive_timer_isr
global yield_isr_wrapper
global syscall_isr_wrapper
global keyboard_isr_wrapper
global mouse_isr_wrapper

extern timer_handler_isr
extern keyboard_handler_isr
extern mouse_handler_isr
extern syscall_handler
extern schedule
extern current_task_cr3

preemptive_timer_isr:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call timer_handler_isr

    push esp
    call schedule
    add esp, 4
    mov esp, eax

    mov ebx, [current_task_cr3]
    mov cr3, ebx

    mov al, 0x20
    out 0x20, al

    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret

yield_isr_wrapper:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call schedule
    add esp, 4
    mov esp, eax

    mov ebx, [current_task_cr3]
    mov cr3, ebx

    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret

syscall_isr_wrapper:
    pusha
    push ds
    push es
    push fs
    push gs

    ; ИСПРАВЛЕНИЕ: СНАЧАЛА пушим аргументы, пока EBX еще цел!
    push esi
    push edx
    push ecx
    push ebx
    push eax

    ; ТЕПЕРЬ безопасно переключаем сегменты на Ядро
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call syscall_handler
    add esp, 20
    
    mov [esp + 44], eax

    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret

keyboard_isr_wrapper:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    call keyboard_handler_isr
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret

mouse_isr_wrapper:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    call mouse_handler_isr
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret

section .bss
align 16
stack_bottom:
resb 16384
stack_top: