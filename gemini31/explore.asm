[bits 32]
call get_base
get_base: pop ebp
sub ebp, get_base

mov eax, 3          ; SYS_CREATE_WIN
mov ebx, 100        ; x
mov ecx, 100        ; y
mov edx, 400        ; w
mov esi, 250        ; h
lea edi, [ebp + str_title]
int 0x80

mov eax, 0          ; SYS_EXIT (Ядро само нарисует файлы внутри окна с таким именем!)
int 0x80
ret

str_title db 'File Explorer', 0