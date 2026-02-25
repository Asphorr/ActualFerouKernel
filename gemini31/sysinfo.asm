[bits 32]
call get_base
get_base: pop ebp
sub ebp, get_base

mov eax, 3          
mov ebx, 200        
mov ecx, 50        
mov edx, 350        
mov esi, 150        
lea edi, [ebp + str_title]
int 0x80
mov [ebp + win_id], eax

; Устанавливаем базу "OS: ModularOS v1.0\nRAM: "
mov eax, 4
mov ebx, [ebp + win_id]
lea esi, [ebp + str_os]
int 0x80

; Узнаем RAM
mov eax, 6
int 0x80
; Переводим RAM в строку
mov ebx, eax
lea ecx, [ebp + tmp_buf]
mov eax, 7
int 0x80
; Дописываем RAM в окно
mov eax, 8
mov ebx, [ebp + win_id]
lea esi, [ebp + tmp_buf]
int 0x80

; Дописываем " bytes\nUptime: "
mov eax, 8
lea esi, [ebp + str_up1]
int 0x80

; Узнаем Аптайм
mov eax, 5
int 0x80
; Переводим Аптайм в строку
mov ebx, eax
lea ecx, [ebp + tmp_buf]
mov eax, 7
int 0x80
; Дописываем Аптайм в окно
mov eax, 8
mov ebx, [ebp + win_id]
lea esi, [ebp + tmp_buf]
int 0x80

; Дописываем " sec"
mov eax, 8
lea esi, [ebp + str_up2]
int 0x80

mov eax, 0
int 0x80
ret

win_id dd 0
str_title db 'System Information', 0
str_os db 'OS: ModularOS v1.0', 10, 'RAM Used: ', 0
str_up1 db ' bytes', 10, 'Uptime: ', 0
str_up2 db ' sec', 0
tmp_buf times 16 db 0