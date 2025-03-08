#include <stdio.h>
#include <stdlib.h>

int GCF(int a, int b) {
    a = abs(a);
    b = abs(b);

    if (a == 0 && b == 0) return 0; 
    if (a == 0) return b;           
    if (b == 0) return a;           

    int min_val = (a < b) ? a : b;  
    int max_gcd = 1;                

    for (int i = 1; i <= min_val; i++) {
        if (a % i == 0 && b % i == 0) {
            max_gcd = i;
        }
    }
    return max_gcd;
}