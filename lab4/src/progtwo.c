#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

typedef int (*tCalculatePrime)(int, int);
typedef int (*tGCF)(int, int);

typedef struct {
    tGCF GCF;
    tCalculatePrime calculatePrime;
    void* library;
} tFuncLibrary;

tFuncLibrary load_library(char* filename) {
    // подгрузка библиотеки
    tFuncLibrary result;
    result.library = dlopen(filename, RTLD_LAZY);
    if (!result.library) {
        fprintf(stderr, "ошибка загрузки библиотеки: %s\n", dlerror());
        return result;
    }

    // загружием функции
    result.calculatePrime = dlsym(result.library, "CalculatePrime");
    result.GCF = dlsym(result.library, "GCF");

    if (!result.calculatePrime || !result.GCF) {
        fprintf(stderr, "Ошибка загрузки функций из библиотеки: %s\n", dlerror());
        dlclose(result.library);
        result.library = NULL;
        return result;
    }

    return result;
}

int main() {
    tFuncLibrary funcLib = load_library("./libImpl1.so");
    if (funcLib.library == NULL) {
        return 1;
    }
    int lib_index = 0;

    int command;
    while (1) {
        printf("Введи команду:\n");
        printf(" 0 -> Library switch\n");
        printf(" 1 -> PrimeCount\n");
        printf(" 2 -> GCF\n");
        printf("-1 -> Ценок\n");
        scanf("%d", &command);

        if (command == -1) {
            break;
        } else if (command == 0) {
            dlclose(funcLib.library);
            lib_index = lib_index == 0 ? 1 : 0;
            funcLib = load_library(lib_index == 0 ? "./libImpl1.so" : "./libImpl2.so");
            if (funcLib.library == NULL) {
                continue;
            }

            printf("заменена библиотека\n");
            printf("эта библиотека: %s\n", lib_index == 0 ? "./libImpl1.so" : "./libImpl2.so");
        } else if (command == 1) {
            int A, B;
            printf("Введи A и B: ");
            scanf("%d %d", &A, &B);
            printf("PrimeCount(%d, %d) = %d\n", A, B, funcLib.calculatePrime(A, B));
            printf("заюзана библиотека: %s\n", lib_index == 0 ? "./libImpl1.so" : "./libImpl2.so");
        } else if (command == 2) {
            int C, D;
            printf("Введи C и D: ");
            scanf("%d %d", &C, &D);
            printf("GCF(%d, %d) = %d\n", C, D, funcLib.GCF(C, D));
            printf("заюзана библиотека: %s\n", lib_index == 0 ? "./libImpl1.so" : "./libImpl2.so");
        } else {
            printf("Нет такого в меню\n");
        }
    }

    dlclose(funcLib.library);
    return 0;
}