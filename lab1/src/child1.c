#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#define MAX_LINE 1000

int main() {
    char line[MAX_LINE];

    // Чтение строк из stdin (перенаправленный pipe1)
    while (read(STDIN_FILENO, line, MAX_LINE) != 0) {
        // Преобразование строки в верхний регистр
        for (int i = 0; line[i]; i++) {
            line[i] = toupper(line[i]);
        }
        
        // Отправка строки в pipe2 (stdout был перенаправлен)
        write(STDOUT_FILENO, line, strlen(line)); // записываем в стандартный поток вывода строку line длины strlen(line)
    }

    return 0;
}
