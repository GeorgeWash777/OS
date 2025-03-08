#include <stdio.h>

// объявлем функций из библиотек
extern int CalculatePrime(int, int);
extern int GCF(int, int);

int main() {
    int command;
    while (1) {
        printf("Введи команду:\n");
        printf(" 1 -> PrimeCount\n");
        printf(" 2 -> GCF\n");
        printf("-1 -> Ценок\n");
        scanf("%d", &command);

        if (command == -1) {
            break;
        } else if (command == 1) {
            int A, B;
            printf("Введи A и B: ");
            scanf("%d %d", &A, &B);
            printf("PrimeCount(%d, %d) = %d\n", A, B, CalculatePrime(A, B));
        } else if (command == 2) {
            int C, D;
            printf("Введи C и D: ");
            scanf("%d %d", &C, &D);
            printf("GCF(%d, %d) = %d\n", C, D, GCF(C, D));
        } else {
            printf("Нет такого в меню\n");
        }
    }
    return 0;
}