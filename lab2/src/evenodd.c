#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define MIN_MERGE 32
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// данные в поток
typedef struct {
    int *array;
    int left;
    int right;
} ThreadData;


//слияние двух отсортированных подмассивов
void merge(int arr[], int l, int m, int r) {
    int len1 = m - l + 1, len2 = r - m;
    int *left = (int *)malloc(len1 * sizeof(int));
    int *right = (int *)malloc(len2 * sizeof(int));

    for (int i = 0; i < len1; i++)
        left[i] = arr[l + i];
    for (int i = 0; i < len2; i++)
        right[i] = arr[m + 1 + i];

    int i = 0, j = 0, k = l;
    while (i < len1 && j < len2) {
        if (left[i] <= right[j])
            arr[k++] = left[i++];
        else
            arr[k++] = right[j++];
    }

    while (i < len1)
        arr[k++] = left[i++];

    while (j < len2)
        arr[k++] = right[j++];

    free(left);
    free(right);
}

// моя сортировка по вики
void evenOddSort(int arr[], int left, int right) {
    int n = right - left + 1;
    for (int p = 1; p < n; p *= 2) {
        for (int k = p; k >= 1; k /= 2) {
            for (int j = k % p; j <= n - 1 - k; j += 2 * k) {
                int max_i = (k - 1 < n - j - k - 1) ? k - 1 : n - j - k - 1;
                for (int i = 0; i <= max_i; i++) {
                    int a = i + j;
                    int b = a + k;
                    if ((a / (p * 2)) == (b /  (p * 2))) {
                        if (arr[a + left] > arr[b + left]) {
                            int temp = arr[a + left];
                            arr[a + left] = arr[b + left];
                            arr[b + left] = temp;
                        }
                    }                    
                }
            }
        }
    }
}


void *evenoddThread(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int *arr = data->array;
    int left = data->left;
    int right = data->right;
    
    evenOddSort(arr, left, right);

    pthread_exit(NULL);
}

void evenodd(int arr[], int n, int maxThreads) {
    if (n < 2) return;

    pthread_t *threads = (pthread_t *)malloc(maxThreads * sizeof(pthread_t));
    ThreadData *threadData = (ThreadData *)malloc(maxThreads * sizeof(ThreadData));

    int chunkSize = n / maxThreads;
    for (int i = 0; i < maxThreads; i++) {
        int start = i * chunkSize;
        int end = MIN(start + chunkSize - 1, n - 1);
        if (start >= n) break;

        threadData[i].array = arr;
        threadData[i].left = start;
        threadData[i].right = end;

        pthread_create(&threads[i], NULL, evenoddThread, (void *)&threadData[i]);
    }

    for (int i = 0; i < maxThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // слияние частей массива
    for (int size = chunkSize; size < n; size = 2 * size) {
        for (int left = 0; left < n; left += 2 * size) {
            int mid = left + size - 1;
            int right = MIN(left + 2 * size - 1, n - 1);
            if (mid < right) {
                merge(arr, left, mid, right);
            }  
        }
    }

    free(threads);
    free(threadData);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Вы не ввели макс колво потоков\n", argv[0]);
        return 1;
    }

    int maxThreads = atoi(argv[1]);
    int arraySize;

    printf("Введите число элементов массива: ");
    scanf("%d", &arraySize);

    int *arr = (int *)malloc(arraySize * sizeof(int));
    // инициализация генератора случайных чисел
    srand(time(NULL));

    // генерация случайных чисел для массива
    for (int i = 0; i < arraySize; i++) {
        arr[i] = rand() % 100; // 0-99
    }

    printf("Оригинал:\n");
    for (int i = 0; i < arraySize; i++) {
       printf("%d ", arr[i]);
    }
    printf("\n");

    struct timeval start, end;
    gettimeofday(&start, NULL);

    evenodd(arr, arraySize, maxThreads);

    gettimeofday(&end, NULL);

    printf("Отсортированный:\n");
    for (int i = 0; i < arraySize; i++) {
       printf("%d ", arr[i]);
    }
    printf("\n");

    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double elapsed = seconds + microseconds * 1e-6;

    printf("Заняло времени: %.6f секунд\n", elapsed);
    printf("Количество потоков: %d\n", maxThreads);

    free(arr);
    return 0;
}