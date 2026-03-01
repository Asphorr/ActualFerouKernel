[BITS 32]

section .text
global _start
global stack_top 
extern kernel_main

_start:
    cli
    mov esp, stack_top
    call kernel_main
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
extern current_task
extern fpu_states_ptr

; ИСПРАВЛЕНИЕ: Берем динамический указатель, который 100% выровнен!
%macro SAVE_FPU_USER 0
    push eax
    push ebx
    mov eax, [fpu_states_ptr]
    mov ebx, [current_task]
    shl ebx, 10 
    add eax, ebx
    fxsave [eax]
    pop ebx
    pop eax
%endmacro

%macro RESTORE_FPU_USER 0
    push eax
    push ebx
    mov eax, [fpu_states_ptr]
    mov ebx, [current_task]
    shl ebx, 10 
    add eax, ebx
    fxrstor [eax]
    pop ebx
    pop eax
%endmacro

%macro SAVE_FPU_KERNEL 0
    push eax
    push ebx
    mov eax, [fpu_states_ptr]
    mov ebx, [current_task]
    shl ebx, 10 
    add eax, ebx
    add eax, 512
    fxsave [eax]
    pop ebx
    pop eax
%endmacro

%macro RESTORE_FPU_KERNEL 0
    push eax
    push ebx
    mov eax, [fpu_states_ptr]
    mov ebx, [current_task]
    shl ebx, 10 
    add eax, ebx
    add eax, 512
    fxrstor [eax]
    pop ebx
    pop eax
%endmacro

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

    mov eax, [esp + 52]
    cmp ax, 0x08
    je .k
    SAVE_FPU_USER
    jmp .s
.k: SAVE_FPU_KERNEL
.s:

    call timer_handler_isr

    push esp
    call schedule
    add esp, 4
    mov esp, eax

    mov ebx, [current_task_cr3]
    mov cr3, ebx

    mov eax, [esp + 52]
    cmp ax, 0x08
    je .rk
    RESTORE_FPU_USER
    jmp .rs
.rk: RESTORE_FPU_KERNEL
.rs:

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

    SAVE_FPU_KERNEL
    push esp
    call schedule
    add esp, 4
    mov esp, eax

    mov ebx, [current_task_cr3]
    mov cr3, ebx

    RESTORE_FPU_KERNEL
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

    push esi
    push edx
    push ecx
    push ebx
    push eax

    mov bx, 0x10
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    SAVE_FPU_USER
    call syscall_handler
    RESTORE_FPU_USER

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
    
    mov eax, [esp + 52]
    cmp ax, 0x08
    je .k
    SAVE_FPU_USER
    jmp .s
.k: SAVE_FPU_KERNEL
.s:
    call keyboard_handler_isr
    
    mov eax, [esp + 52]
    cmp ax, 0x08
    je .rk
    RESTORE_FPU_USER
    jmp .rs
.rk: RESTORE_FPU_KERNEL
.rs:
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
    
    mov eax, [esp + 52]
    cmp ax, 0x08
    je .k
    SAVE_FPU_USER
    jmp .s
.k: SAVE_FPU_KERNEL
.s:
    call mouse_handler_isr
    
    mov eax, [esp + 52]
    cmp ax, 0x08
    je .rk
    RESTORE_FPU_USER
    jmp .rs
.rk: RESTORE_FPU_KERNEL
.rs:
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