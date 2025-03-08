#include <stdio.h>
#include <stdlib.h>

int GCF(int a, int b) {
    a = abs(a); // тока для неотрицательных чисел
    b = abs(b);
    
    while (b != 0) {
        int remainder = a % b; // сстаток от деления
        a = b;
        b = remainder;
    }
    return a; // b == 0, НОД = a
}