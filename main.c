#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>

int REASONABLE_THREAD_MAX = 300;

/**/
double average (double a, double b, double c, double d) {
    return (a+b+c+d)/4.0;
}

void printArray (const double *inputArray, int dimension) {
    int i, j;
    for (i = 0; i < dimension; i++) {
        for (j = 0; j < dimension; j++) {
            printf("%lf ", inputArray[i*dimension + j]);
        }
        printf("\n");
    }
}

void copyArray(double *dest, const double *src, int dimension) {
    int i, j;
    for (i = 0; i < dimension; i++) {
        for (j = 0; j < dimension; j++) {
            dest[i*dimension + j] = src[i*dimension + j];
        }
    }
}

struct arg_struct {
    int dimension;
    double *inputArray;
    double *copy;
    double precision;
    int numThreads;
};

void *solver(void * arguments) {
    struct arg_struct *args = (struct arg_struct *)arguments;
    printf("Dimension: %d\n", args -> dimension);

    int self = (int)pthread_self();
    int start_i, end_i, remainder, rowsToUse,  numRows;
    numRows = args->dimension - 2;
    rowsToUse = (int)floor(numRows/args->numThreads);
    remainder = (numRows % args->numThreads);
    printf("remainder %d\n", remainder);
    printf("rows to use %d\n", rowsToUse);
    if (remainder - self >= 0) {
        start_i = (self - 1)*rowsToUse + (self - 1) + 1;
        rowsToUse++;
    }
    else {
        start_i = (self - 1)*rowsToUse + remainder + 1;
    }
    end_i = start_i + rowsToUse;

    double a, b, c, d, av, current, diff;
    int test = 0;
    int stop = 0;

    printf("thread id: %d, ", self);
    printf("start: %d, end: %d\n", start_i, end_i);
    while (stop == 0) {
        test++;
        printf("round: %d\n", test);
        stop = 1;
        for (int i = start_i; i < end_i; i++) {
            for (int j = 1; j < args -> dimension-1; j++) {
                current = args -> inputArray[i*args -> dimension + j];
                a = *((args -> inputArray+(i-1)*args -> dimension) + j);
                b = *((args -> inputArray+i*args -> dimension) + (j-1));
                c = *((args -> inputArray+(i+1)*args -> dimension) + j);
                d = *((args -> inputArray+i*args -> dimension) + (j+1));
                av = average(a, b, c, d);
                diff = fabs(av - current);
                if (diff > args -> precision) {
//                    printf("changed array[%d, %d] from %lf to %lf\n", i, j, current, av);
                    args -> copy[i*args -> dimension + j] = av;
                    stop = 0;
                }
            }
        }
        printf("\nInput: \n");
        printArray(args -> inputArray, args -> dimension);
        printf("\nCopy: \n");
        printArray(args -> copy, args -> dimension);
        printf("\n");
        copyArray(args -> inputArray, args -> copy, args -> dimension);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        int numThreads = atoi(argv[1]);
        if ((numThreads <= 0) || (numThreads > REASONABLE_THREAD_MAX)) {
            printf("invalid argument for thread count\n");
            exit(EXIT_FAILURE);
        }

        pthread_t * thread = malloc(sizeof(pthread_t)*numThreads);
        if (thread == NULL) {
            printf("out of memory\n");
            exit(EXIT_FAILURE);
        }

        int i;
        double arr[25] = {5, 7, 8, 9, 11, 2, 2, 7, 5, 7, 9, 6, 1, 8, 2, 8, 3, 9, 4, 3, 3, 4, 7, 8, 9};
        double arr2[25] = {5, 7, 8, 9, 11, 2, 2, 7, 5, 7, 9, 6, 1, 8, 2, 8, 3, 9, 4, 3, 3, 4, 7, 8, 9};
        struct arg_struct args;
        args.dimension = 5;
        args.precision = 0;
        args.inputArray = arr;
        args.copy = arr2;
        args.numThreads = numThreads;

        for (i = 0; i < numThreads; i++) {
            if (pthread_create (&thread[i], NULL, solver, (void*) &args ) != 0) {
                printf("Error. \n");
                exit(EXIT_FAILURE);
            }
        }
        for (i = 0; i < numThreads; i++) {
            pthread_join(thread[i], NULL);
        }
    }
}