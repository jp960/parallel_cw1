#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <mem.h>
#include <pthread_time.h>

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
    int *currentStop;
    int *nextStop;
};

void sequentialSolver(void *arguments) {
    struct arg_struct *args = (struct arg_struct *) arguments;
    double a, b, c, d, av, current, diff;
    int stop = 0;
    int count = 0;
    int run = 0;
    double *localOne = args->currentArray;
    double *localTwo = args->nextArray;

    while (stop == 0) {
        stop = 1;
        for (int i = 1; i < args->dimension - 1; i++) {
            for (int j = 1; j < args->dimension - 1; j++) {
                current = localOne[i * args->dimension + j];
                a = *((localOne + (i - 1) * args->dimension) + j);
                b = *((localOne + i * args->dimension) + (j - 1));
                c = *((localOne + (i + 1) * args->dimension) + j);
                d = *((localOne + i * args->dimension) + (j + 1));
                av = average(a, b, c, d);
                diff = fabs(av - current);
                localTwo[i * args->dimension + j] = av;
                if (stop == 1 && diff > args->precision) {
                    stop = 0;
                }
            }
        }
        run++;
        if (count == 0) {
            localOne = args->nextArray;
            localTwo = args->currentArray;
            count++;
        }
        else {
            localOne = args->currentArray;
            localTwo = args->nextArray;
            count--;
        }
    }
}

void *parallelSolver(void *arguments) {
    struct arg_struct *args = (struct arg_struct *) arguments;

    int count = 0;
    int self = (int) pthread_self();
    int start_i = 0;
    int end_i = 0;
    int run = 0;
    int *localStopOne = args->currentStop;
    int *localStopTwo = args->nextStop;
    double *localOne = args->currentArray;
    double *localTwo = args->nextArray;
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
                if (*localStopOne == 1 && diff > args->precision) {
                    pthread_mutex_lock(&lock);
                    *localStopOne = 0;
                    pthread_mutex_unlock(&lock);
                }
            }
        }
        pthread_barrier_wait(&barrier);
        if (self == 1) {
            run++;
        }
        if (*localStopOne == 1) {
            pthread_exit(0);
            break;
        }
        if (*localStopTwo != 1) {
            pthread_mutex_lock(&lock);
            *localStopTwo = 1;
            pthread_mutex_unlock(&lock);
        }
        pthread_barrier_wait(&barrier);
        if (count == 0) {
            localOne = args->nextArray;
            localTwo = args->currentArray;
            localStopOne = args->currentStop;
            localStopTwo = args->nextStop;
            count++;
        }
        else {
            localOne = args->currentArray;
            localTwo = args->nextArray;
            localStopOne = args->nextStop;
            localStopTwo = args->currentStop;
            count--;
        }
    }
}

int precisionTest(void *three) {
    struct arg_struct *args = (struct arg_struct *) three;
    int size = args->dimension*args->dimension;
    int count = 0;
    double diff;
    for (int i = 0;i <size; i++) {
        diff = fabs(args->currentArray[i] - args->nextArray[i]);
        if (diff > args->precision) {
            count++;
        }
    }
    return count;
}

void correctnessTest(double seqArray[], void * two) {
    struct arg_struct *args2 = (struct arg_struct *) two;
    int size = args2->dimension*args2->dimension;
    double results[size];
    memset(results, 0, sizeof(results));
    int count = 0;
    double diff;

    for (int i = 0; i < args2->dimension; i++) {
        for (int j = 0; j < args2->dimension; j++) {
            diff = fabs(seqArray[i * args2->dimension + j] - args2->currentArray[i * args2->dimension + j]);
            if (diff > 0.000001) {
                results[i * args2->dimension + j] = diff;
                count++;
            }
        }
    }
    if(count != 0){
        printf("Not correct in %d places\n", count);
        printArray(results, args2->dimension);
    }
    else {
        printf("Correct\n");
    }
}

void runSequential(double arr3[], double arr4[], int dimension, double precision, int numThreads, char * filename) {
    int arraySize = dimension * dimension;
    FILE * fpRead = fopen("./numbers.txt", "r+");
    setArray(arr3, arr4, arraySize, fpRead);
    fclose(fpRead);

    struct timespec begin, end;
    time_t time_spent;

    struct arg_struct seqArgs;
    seqArgs.dimension = dimension;
    seqArgs.precision = precision;
    seqArgs.currentArray = arr3;
    seqArgs.nextArray = arr4;
    seqArgs.numThreads = numThreads;
    seqArgs.currentStop = malloc(sizeof(int));
    seqArgs.nextStop = malloc(sizeof(int));

    clock_gettime(CLOCK_MONOTONIC, &begin);
    sequentialSolver(&seqArgs);
    clock_gettime(CLOCK_MONOTONIC, &end);

    time_spent = (1000000000L * (end.tv_sec - begin.tv_sec)) + end.tv_nsec - begin.tv_nsec;
    printf("Sequential run complete\n");
    FILE * fpWrite = fopen(filename, "w+");
    fprintf(fpWrite, "Time: %llu nanoseconds.\n", (unsigned long long int) time_spent);
    int count = precisionTest(&seqArgs);
    if(count != 0){
        fprintf(fpWrite, "Not precise in %d places\n", count);
    }
    else {
        fprintf(fpWrite, "Precise\n");
    }
    for (int i = 0; i < arraySize; i++) {
        fprintf(fpWrite, "%lf,", seqArgs.currentArray[i]);
    }
    fclose(fpWrite);
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        unsigned int numThreads = (unsigned int)atoi(argv[1]);
        int dimension = atoi(argv[2]);
        double precision = atof(argv[3]);
        int arraySize = dimension*dimension;

        struct timespec begin, end;
        time_t time_spent;

        double arr1[arraySize];
        double arr2[arraySize];
        double arr3[arraySize];
        double arr4[arraySize];
        FILE * fp1 = fopen("./numbers.txt", "r+");
        setArray(arr1, arr2, arraySize, fp1);
        fclose(fp1);

        char seqFilename[64];
        sprintf(seqFilename, "seqOut_%d.txt", dimension);
        if (numThreads == 1) {
            runSequential(arr3, arr4, dimension, precision, numThreads, seqFilename);
        }

        double seqArray1[arraySize];
        double seqArray2[arraySize];
        char firstLine[64];
        char precise[40];
        FILE * fpReadSeqArray = fopen(seqFilename, "r+");
        fgets(firstLine, 64, fpReadSeqArray);
        fgets(precise, 64, fpReadSeqArray);
        setArray(seqArray1, seqArray2, arraySize, fpReadSeqArray);
        fclose(fpReadSeqArray);

        clock_gettime(CLOCK_MONOTONIC, &begin);
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

        struct arg_struct parallelArgs;
        parallelArgs.dimension = dimension;
        parallelArgs.precision = precision;
        parallelArgs.currentArray = arr1;
        parallelArgs.nextArray = arr2;
        parallelArgs.numThreads = numThreads;
        parallelArgs.currentStop = malloc(sizeof(int));
        parallelArgs.nextStop = malloc(sizeof(int));

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
        clock_gettime(CLOCK_MONOTONIC, &end);
        time_spent = (1000000000L * (end.tv_sec - begin.tv_sec)) + end.tv_nsec - begin.tv_nsec;

        printf("Array size %d by %d\n", dimension, dimension);
        printf("Sequential run\n");
        printf("%s", firstLine);
        printf("%s", precise);
        printf("Parallel run\n");
        printf("Number of Threads: %d\n", numThreads);
        printf("Time: %llu nanoseconds.\n", (unsigned long long int) time_spent);
        int count = precisionTest(&parallelArgs);
        if(count != 0){
            printf("Not precise in %d places\n", count);
        }
        else {
            printf("Precise\n");
        }
        correctnessTest(seqArray1, &parallelArgs);
    }
}