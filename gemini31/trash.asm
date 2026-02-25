[bits 32]
call get_base
get_base: pop ebp
sub ebp, get_base

mov eax, 3          
mov ebx, 150        
mov ecx, 200        
mov edx, 300        
mov esi, 150        
lea edi, [ebp + str_title]
int 0x80
mov [ebp + win_id], eax

mov eax, 4          ; SYS_SET_CONTENT
mov ebx, [ebp + win_id]
lea esi, [ebp + str_txt]
int 0x80

mov eax, 0
int 0x80
ret

win_id dd 0
str_title db 'Recycle Bin', 0
str_txt db 'The trash is completely empty.', 0