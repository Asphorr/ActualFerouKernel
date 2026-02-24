[bits 16]
[extern main]
[extern keyboard_handler_c]
[extern panic_handler_c]
[extern schedule]           ; Наш Си-планировщик задач
[extern syscall_handler_c]  ; Наш обработчик системных вызовов (Сисколлов)
[extern mouse_handler_c]    ; <-- Добавь эту строчку!

global _start
global load_idt
global isr0_wrapper         ; Исключение 0x00 (Деление на ноль)
global isr1_wrapper         ; Прерывание 0x21 (Клавиатура)
global irq0_wrapper         ; Прерывание 0x20 (Таймер)
global irq12_wrapper        ; Прерывание 0x2C (Мышь)
global isr128_wrapper       ; Прерывание 0x80 (Системные вызовы)

; --- ЭКСПОРТИРУЕМ АДРЕС ВИДЕОПАМЯТИ ДЛЯ СИ ---
global vbe_framebuffer
vbe_framebuffer dd 0

_start:
    ; ==================================================
    ; Включаем VESA VBE (800x600, 32-bit True Color)
    ; ==================================================
    ; 1. Получаем информацию о режиме 0x115 (800x600x32)
    mov ax, 0x4F01
    mov cx, 0x115          
    mov di, 0x8000         ; Временный буфер для структуры
    int 0x10
    
    ; 2. Достаем физический адрес фреймбуфера (offset 40) и сохраняем его
    mov eax, dword [0x8000 + 0x28] 
    mov [vbe_framebuffer], eax
    
    ; 3. Устанавливаем режим с флагом Linear Framebuffer (0x4000)
    mov ax, 0x4F02
    mov bx, 0x4115
    int 0x10
    ; ==================================================

    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp 0x08:init_pm


[bits 32]
init_pm:
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebp, 0x90000
    mov esp, ebp

    call main
    jmp $

load_idt:
    mov eax, [esp + 4]
    lidt [eax]
    sti
    ret

; ==========================================
; ОБЕРТКИ ДЛЯ ПРЕРЫВАНИЙ (ИСКЛЮЧЕНИЯ И ЖЕЛЕЗО)
; ==========================================

isr0_wrapper:               ; Исключение 0x00 (Деление на ноль)
    pusha
    call panic_handler_c
    popa
    iretd

isr1_wrapper:               ; Прерывание 0x21 (Клавиатура)
    pusha
    call keyboard_handler_c
    popa
    iretd

; --- ОБРАБОТЧИК ТАЙМЕРА (С ПЕРЕКЛЮЧЕНИЕМ ЗАДАЧ) ---
irq0_wrapper:               ; Прерывание 0x20 (Таймер - 100 раз в секунду)
    pusha                   ; 1. Сохраняем регистры текущей задачи в ЕЁ стек
    
    push esp                ; 2. Передаем текущий адрес стека (ESP) как аргумент в Си-функцию
    call schedule           ; 3. Вызываем Планировщик. Он вернет в EAX адрес НОВОГО стека!
    add esp, 4              ; Убираем аргумент
    
    mov esp, eax            ; 4. МАГИЯ! МЕНЯЕМ СТЕК ПРОЦЕССОРА НА СТЕК ДРУГОЙ ЗАДАЧИ!

    mov al, 0x20
    out 0x20, al            ; Отправляем EOI контроллеру прерываний
    
    popa                    ; 5. Восстанавливаем регистры уже ИЗ НОВОГО стека
    iretd                   ; 6. Прыгаем в код НОВОЙ задачи!

irq12_wrapper:              ; Прерывание 0x2C (Мышь)
    pusha
    call mouse_handler_c    ; Вызываем умный обработчик на Си!
    popa
    iretd

; ==========================================
; ШЛЮЗ ДЛЯ СИСТЕМНЫХ ВЫЗОВОВ (API ПРОГРАММ)
; ==========================================
isr128_wrapper:             ; Прерывание 0x80 (128)
    pusha                   ; Сохраняем все регистры программы
    push esp                ; Передаем указатель на стек как аргумент
    call syscall_handler_c  ; Вызываем обработчик ядра
    add esp, 4              ; Очищаем стек
    popa                    ; Восстанавливаем регистры
    iretd

; --- GDT ---
gdt_start:
    dd 0x0, 0x0
gdt_code:
    dw 0xffff, 0x0
    db 0x0, 10011010b, 11001111b, 0x0
gdt_data:
    dw 0xffff, 0x0
    db 0x0, 10010010b, 11001111b, 0x0
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start