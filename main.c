#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <mem.h>

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

void setArray(double *one, double *two, int size, FILE * fp) {
    for (int k = 0; k < size; k++){
        fscanf(fp, "%lf,", &one[k]);
        two[k] = one[k];
    }
}

void setThreadArraySections(int *start_i, int *end_i, int dimension, int numThreads, int self){
    int remainder, rowsToUse, numRows;

    numRows = dimension - 2;
    rowsToUse = (int) floor(numRows / numThreads);
    remainder = (numRows % numThreads);
    if (remainder - self >= 0) {
        *start_i = (self - 1) * rowsToUse + (self - 1) + 1;
        rowsToUse++;
    } else {
        *start_i = (self - 1) * rowsToUse + remainder + 1;
    }
    *end_i = *start_i + rowsToUse;
}

struct arg_struct {
    int dimension;
    double *currentArray;
    double *nextArray;
    double precision;
    int numThreads;
    int currentStop;
};

void sequentialSolver(void *arguments) {
    struct arg_struct *args = (struct arg_struct *) arguments;
    double a, b, c, d, av, current, diff;
    int stop = 0;

    while (stop == 0) {
        stop = 1;
        for (int i = 1; i < args->dimension - 1; i++) {
            for (int j = 1; j < args->dimension - 1; j++) {
                current = args->currentArray[i * args->dimension + j];
                a = *((args->currentArray + (i - 1) * args->dimension) + j);
                b = *((args->currentArray + i * args->dimension) + (j - 1));
                c = *((args->currentArray + (i + 1) * args->dimension) + j);
                d = *((args->currentArray + i * args->dimension) + (j + 1));
                av = average(a, b, c, d);
                diff = fabs(av - current);
                args->nextArray[i * args->dimension + j] = av;
                if (diff > args->precision) {
                    stop = 0;
                }
            }
        }
        double * temp = args->currentArray;
        args->currentArray = args->nextArray;
        args->nextArray = temp;
    }
//    printf("\nSequential Answer: \n");
//    printArray(args->currentArray, args->dimension);
//    printf("\n");
}

void *parallelSolver(void *arguments) {
    struct arg_struct *args = (struct arg_struct *) arguments;

    int count = 0;
    int self = (int) pthread_self();
    int start_i = 0;
    int end_i = 0;
    int currentStop = args->currentStop;
    int nextStop = 0;
    double * localOne = args->currentArray;
    double * localTwo = args->nextArray;
    double a, b, c, d, av, current, diff;

    setThreadArraySections(&start_i, &end_i, args->dimension, args->numThreads, self);

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
                    pthread_mutex_lock(&lock);
                    currentStop = 0;
                    pthread_mutex_unlock(&lock);
                }
            }
        }
        pthread_barrier_wait(&barrier);
        if (currentStop == 1) {
            if (self == 1) {
//                printf("\nParallel Answer: \n");
//                printArray(localOne, args->dimension);
//                printf("\n");
            }
            pthread_exit(0);
            break;
        }
        nextStop = 1;

        if (count == 0) {
            localOne = args->nextArray;
            localTwo = args->currentArray;
            currentStop = nextStop;
            count++;
        }
        else {
            localOne = args->currentArray;
            localTwo = args->nextArray;
            currentStop = nextStop;
            count--;
        }
    }
}

void precisionTest(void *arguments) {
    struct arg_struct *args = (struct arg_struct *) arguments;
    int size = args->dimension*args->dimension;
    int count = 0;
    double diff;
    for (int i = 0;i <size; i++) {
        diff = fabs(args->currentArray[i] - args->nextArray[i]);
        if (diff > args->precision) {
            count++;
        }
    }
    if(count != 0){
        printf("Not precise in %d places\n", count);
    }
    else {
        printf("Precise\n");
    }
}

void correctnessTest(void * one, void * two) {
    struct arg_struct *args1 = (struct arg_struct *) one;
    struct arg_struct *args2 = (struct arg_struct *) two;
    int size = args1->dimension*args1->dimension;
    double results[size];
    memset(results, 0, sizeof(results));
    int count = 0;
    double diff;

    for (int i = 0; i < args1->dimension; i++) {
        for (int j = 0; j < args1->dimension; j++) {
            diff = fabs(args1->currentArray[i * args1->dimension + j] - args2->nextArray[i * args2->dimension + j]);
            if (diff != 0) {
                results[i * args1->dimension + j] = diff;
                count++;
            }
        }
    }
    if(count != 0){
        printf("Not correct in %d places\n", count);
        printArray(results, args1->dimension);
    }
    else {
        printf("Correct\n");
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
        seqArgs.currentArray = arr1;
        seqArgs.nextArray = arr2;
        seqArgs.numThreads = numThreads;
        seqArgs.currentStop = 0;

        sequentialSolver(&seqArgs);
        precisionTest(&seqArgs);

        struct arg_struct parallelArgs;
        parallelArgs.dimension = dimension;
        parallelArgs.precision = 0.00001;
        parallelArgs.currentArray = arr1;
        parallelArgs.nextArray = arr2;
        parallelArgs.numThreads = numThreads;
        parallelArgs.currentStop = 0;

        int i;
        for (i = 0; i < numThreads; i++) {
            if (pthread_create(&thread[i], NULL, parallelSolver, (void *) &parallelArgs) != 0) {
                printf("Error. \n");
                exit(EXIT_FAILURE);
            }
        }
        for (i = 0; i < numThreads; i++) {
            pthread_join(thread[i], NULL);
        }
        pthread_mutex_destroy(&lock);
        precisionTest(&parallelArgs);
        correctnessTest(&seqArgs, &parallelArgs);
    }
}