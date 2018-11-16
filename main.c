#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

int REASONABLE_THREAD_MAX = 300;
pthread_mutex_t lock;
pthread_barrier_t barrier;


/**/
double average(double a, double b, double c, double d) {
    return (a + b + c + d) / 4.0;
}

void printArray(const double *current, int dimension) {
    int i, j;
    for (i = 0; i < dimension; i++) {
        for (j = 0; j < dimension; j++) {
            printf("%lf ", current[i * dimension + j]);
        }
        printf("\n");
    }
}

void copyArray(double *dest, const double *src, int dimension) {
    int i, j;
    for (i = 0; i < dimension; i++) {
        for (j = 0; j < dimension; j++) {
            dest[i * dimension + j] = src[i * dimension + j];
        }
    }
}

void setArray(double *one, double *two, int size, FILE * fp) {
    for (int k = 0; k < size; k++){
        fscanf(fp, "%lf,", &one[k]);
        two[k] = one[k];
    }
}

struct arg_struct {
    int dimension;
    double *one;
    double *two;
    double precision;
    int numThreads;
    int stop;
};

void *solver(void *arguments) {
    struct arg_struct *args = (struct arg_struct *) arguments;
    printf("Dimension: %d\n", args->dimension);

    int self = (int) pthread_self();
    int start_i, end_i, remainder, rowsToUse, numRows;

    numRows = args->dimension - 2;
    rowsToUse = (int) floor(numRows / args->numThreads);
    remainder = (numRows % args->numThreads);
    printf("remainder %d\n", remainder);
    printf("rows to use %d\n", rowsToUse);
    if (remainder - self >= 0) {
        start_i = (self - 1) * rowsToUse + (self - 1) + 1;
        rowsToUse++;
    } else {
        start_i = (self - 1) * rowsToUse + remainder + 1;
    }
    end_i = start_i + rowsToUse;

    double a, b, c, d, av, current, diff;

    printf("thread id: %d, ", self);
    printf("start: %d, end: %d\n", start_i, end_i);
    while (1) {
        for (int i = start_i; i < end_i; i++) {
            for (int j = 1; j < args->dimension - 1; j++) {
                current = args->one[i * args->dimension + j];
                a = *((args->one + (i - 1) * args->dimension) + j);
                b = *((args->one + i * args->dimension) + (j - 1));
                c = *((args->one + (i + 1) * args->dimension) + j);
                d = *((args->one + i * args->dimension) + (j + 1));
                av = average(a, b, c, d);
                diff = fabs(av - current);
                if (diff > args->precision) {
                    args->two[i * args->dimension + j] = av;
                    if(i==1 && j==1){
//                        printf("%d, %d: %lf thread id: %d\n", i, j, args->two[i * args->dimension + j], self);
//                        printf("diff: %lf thread id: %d\n", diff, self);
                        printf("one: %lf   two: %lf   diff: %lf\n ", args->one[i * args->dimension + j], args->two[i * args->dimension + j], diff);
                    }
                    pthread_mutex_lock(&lock);
                    args->stop = 0;
                    pthread_mutex_unlock(&lock);
                }
            }
        }
        pthread_barrier_wait(&barrier);
        if (self == 1) {
//            copyArray(args->one, args->two, args->dimension);
            memcpy(args->one, args->two, (sizeof(double)*args->dimension*args->dimension));
//            printf("swap arrays thread id: %d\n", self);
//            double *temp = args->one;
//            args->one = args->two;
//            args->two = temp;
        }
        if (args->stop == 1) {
            printf("\nshould stop here\n");
            if (self == 1) {
                printf("\nstopped input: \n");
                printArray(args->two, args->dimension);
                printf("\n");
            }
            pthread_exit(0);
            break;
        }
        pthread_barrier_wait(&barrier);
//        printf("stop before: %d thread id: %d\n", args->stop, self);
        args->stop = 1;
        pthread_barrier_wait(&barrier);
//        printf("stop after: %d thread id: %d\n", args->stop, self);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        unsigned int numThreads = (unsigned int)atoi(argv[1]);
        int dimension = atoi(argv[2]);
        int arraySize = dimension*dimension;

        pthread_t *thread = malloc(sizeof(pthread_t) * numThreads);
        if (thread == NULL) {
            printf("out of memory\n");
            exit(EXIT_FAILURE);
        }

        if (pthread_mutex_init(&lock, NULL) != 0) {
            printf("mutex init failed\n");
            exit(EXIT_FAILURE);
        }

        if (pthread_barrier_init(&barrier, NULL, numThreads) != 0) {
            printf("barrier init failed\n");
            exit(EXIT_FAILURE);
        }

        int i;
        double ar[25] = {5, 7, 8, 9, 11, 2, 2, 7, 5, 7, 9, 6, 1, 8, 2, 8, 3, 9, 4, 3, 3, 4, 7, 8, 9};
        double ar2[25] = {5, 7, 8, 9, 11, 2, 2, 7, 5, 7, 9, 6, 1, 8, 2, 8, 3, 9, 4, 3, 3, 4, 7, 8, 9};
        double arr1[arraySize];
        double arr2[arraySize];


        FILE * fp = fopen("./numbers.txt", "r+");
        setArray(arr1, arr2, arraySize, fp);

        struct arg_struct args;
        args.dimension = dimension;
        args.precision = 0.001;
        args.one = arr1;
        args.two = arr2;
        args.numThreads = numThreads;
        args.stop = 0;

        for (i = 0; i < numThreads; i++) {
            if (pthread_create(&thread[i], NULL, solver, (void *) &args) != 0) {
                printf("Error. \n");
                exit(EXIT_FAILURE);
            }
        }
        for (i = 0; i < numThreads; i++) {
            pthread_join(thread[i], NULL);
        }
        pthread_mutex_destroy(&lock);
    }
}