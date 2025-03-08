#include <stdio.h>
#include <math.h>

// Решето Эратосфена
int CalculatePrime(int A, int B) {
    if (B < 2)
        return 0;
    int size = B + 1;
    int sieve[size];
    for (int i = 0; i < size; i++) sieve[i] = 1;

    sieve[0] = sieve[1] = 0;
    for (int i = 2; i * i <= B; i++) {
        if (sieve[i]) {
            for (int j = i * i; j <= B; j += i) {
                sieve[j] = 0;
            }
        }
    }

    int count = 0;
    for (int i = A; i <= B; i++) {
        if (sieve[i]) count++;
    }

    return count;
}