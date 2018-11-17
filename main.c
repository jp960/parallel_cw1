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

//void copyArray(double *dest, const double *src, int dimension) {
//    int i, j;
//    for (i = 0; i < dimension; i++) {
//        for (j = 0; j < dimension; j++) {
//            dest[i * dimension + j] = src[i * dimension + j];
//        }
//    }
//}

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

void sequentialSolver(void *arguments) {
    struct arg_struct *args = (struct arg_struct *) arguments;
    double a, b, c, d, av, current, diff;
    int stop = 0;

    while (stop == 0) {
        stop = 1;
        for (int i = 1; i < args->dimension - 1; i++) {
            for (int j = 1; j < args->dimension - 1; j++) {
                current = args->one[i * args->dimension + j];
                a = *((args->one + (i - 1) * args->dimension) + j);
                b = *((args->one + i * args->dimension) + (j - 1));
                c = *((args->one + (i + 1) * args->dimension) + j);
                d = *((args->one + i * args->dimension) + (j + 1));
                av = average(a, b, c, d);
                diff = fabs(av - current);
                args->two[i * args->dimension + j] = av;
                if (diff > args->precision) {
                    stop = 0;
                }
            }
        }
        double * temp = args->one;
        args->one = args->two;
        args->two = temp;
    }
    printf("\nSequential Answer: \n");
    printArray(args->one, args->dimension);
    printf("\n");
}

void *solver(void *arguments) {
    struct arg_struct *args = (struct arg_struct *) arguments;
    printf("Dimension: %d\n", args->dimension);

    int count = 0;

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

    double * localOne = args->one;
    double * localTwo = args->two;

    double a, b, c, d, av, current, diff;

    printf("thread id: %d, ", self);
    printf("start: %d, end: %d\n", start_i, end_i);
    while (1) {
        for (int i = start_i; i < end_i; i++) {
            for (int j = 1; j < args->dimension - 1; j++) {
                current = localOne[i * args->dimension + j];
                a = *((localOne + (i - 1) * args->dimension) + j);
                b = *((localOne + i * args->dimension) + (j - 1));
                c = *((localOne + (i + 1) * args->dimension) + j);
                d = *((localOne + i * args->dimension) + (j + 1));
                av = average(a, b, c, d);
                diff = fabs(av - current);
                localTwo[i * args->dimension + j] = av;
                if (diff > args->precision) {
//                    printf("Thread id: %d \n", self);
//                    printf("%d %lf\n", args->stop, diff);
                    pthread_mutex_lock(&lock);
                    args->stop = 0;
                    pthread_mutex_unlock(&lock);
                }
            }
        }
        pthread_barrier_wait(&barrier);
        if (args->stop == 1) {
            if (self == 1) {
                printf("\nParallel Answer: \n");
                printArray(localOne, args->dimension);
                printf("\n");
            }
            pthread_exit(0);
            break;
        }

        if (count == 0) {
            localOne = args->two;
            localTwo = args->one;
            count++;
        }
        else {
            localOne = args->one;
            localTwo = args->two;
            count--;
        }
//        if (self == 1) {
//            memcpy(localOne, localTwo, (sizeof(double)*args->dimension*args->dimension));
//        }
        pthread_barrier_wait(&barrier);
        args->stop = 1;
        pthread_barrier_wait(&barrier);
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

        double arr1[arraySize];
        double arr2[arraySize];


        FILE * fp = fopen("./numbers.txt", "r+");
        setArray(arr1, arr2, arraySize, fp);

        struct arg_struct seqArgs;
        seqArgs.dimension = dimension;
        seqArgs.precision = 0.00001;
        seqArgs.one = arr1;
        seqArgs.two = arr2;
        seqArgs.numThreads = numThreads;
        seqArgs.stop = 0;

        sequentialSolver(&seqArgs);

        struct arg_struct parallelArgs;
        parallelArgs.dimension = dimension;
        parallelArgs.precision = 0.00001;
        parallelArgs.one = arr1;
        parallelArgs.two = arr2;
        parallelArgs.numThreads = numThreads;
        parallelArgs.stop = 0;

        int i;
        for (i = 0; i < numThreads; i++) {
            if (pthread_create(&thread[i], NULL, solver, (void *) &parallelArgs) != 0) {
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