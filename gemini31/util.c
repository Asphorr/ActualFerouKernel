#include "util.h"

void memory_copy(char* source, char* dest, int no_bytes) {
    for (int i = 0; i < no_bytes; i++) { dest[i] = source[i]; }
}

void int_to_string(int n, char str[]) {
    int i = 0, sign = n;
    if (n == 0) { str[i++] = '0'; str[i] = '\0'; return; }
    if (sign < 0) n = -n;
    while (n > 0) { str[i++] = n % 10 + '0'; n /= 10; }
    if (sign < 0) str[i++] = '-';
    str[i] = '\0';
    int start = 0, end = i - 1;
    while (start < end) {
        char temp = str[start]; str[start] = str[end]; str[end] = temp;
        start++; end--;
    }
}

// Перевод числа в шестнадцатеричную строку (например, 255 -> "0xFF")
void int_to_hex(unsigned int n, char str[]) {
    str[0] = '0';
    str[1] = 'x';
    int len = 2;

    if (n == 0) {
        str[len++] = '0';
        str[len] = '\0';
        return;
    }

    char hex_digits[16];
    int num_digits = 0;
    
    // Вытаскиваем цифры с конца
    while (n > 0) {
        int rem = n % 16;
        if (rem < 10) hex_digits[num_digits++] = rem + '0';
        else hex_digits[num_digits++] = (rem - 10) + 'A';
        n /= 16;
    }

    // Переворачиваем и записываем в итоговую строку
    for (int i = num_digits - 1; i >= 0; i--) {
        str[len++] = hex_digits[i];
    }
    str[len] = '\0';
}

int str_compare(char* str1, char* str2) {
    int i = 0;
    while (str1[i] == str2[i]) {
        if (str1[i] == '\0') return 1;
        i++;
    }
    return 0;
}
// Возвращает 1, если строка str начинается со слова prefix
int str_starts_with(char* str, char* prefix) {
    int i = 0;
    while(prefix[i] != '\0') {
        if (str[i] != prefix[i]) return 0; // Нашли отличие
        i++;
    }
    return 1; // Все буквы совпали
}

int str_length(char* str) {
    int len = 0;
    while(str[len] != '\0') len++;
    return len;
}