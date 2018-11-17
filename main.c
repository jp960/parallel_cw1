#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <pthread_time.h>

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
    printf("\n");
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
    int *stopOne;
    int *stopTwo;
};


void *solver(void *arguments) {


    struct timespec begin, end;
    long time_spent;

    struct arg_struct *args = (struct arg_struct *) arguments;

    int self = (int) pthread_self();
    int start_i, end_i, remainder, rowsToUse, numRows;

    numRows = args->dimension - 2;
    rowsToUse = (int) floor(numRows / args->numThreads);
    remainder = (numRows % args->numThreads);
    if (remainder - self >= 0) {
        start_i = (self - 1) * rowsToUse + (self - 1) + 1;
        rowsToUse++;
    } else {
        start_i = (self - 1) * rowsToUse + remainder + 1;
    }
    end_i = start_i + rowsToUse;

    double a, b, c, d, av, current, diff;
    double *localOne = args->one;
    double *localTwo = args->two;

    int  *stopNext = args->stopTwo;
    int  *stopCurrent  = args->stopOne;
    int dimension = args->dimension;
    int count = 0;
    int test = 0;
    while (1) {

        for (int i = start_i; i < end_i; i++) {
            for (int j = 1; j < dimension - 1; j++) {

                current = localOne[i * dimension + j];
                a = *((localOne + (i - 1) * dimension) + j);
                b = *((localOne + i * dimension) + (j - 1));
                c = *((localOne + (i + 1) * dimension) + j);
                d = *((localOne + i * dimension) + (j + 1));
                av = average(a, b, c, d);

                diff = fabs(av - current);
                localTwo[i * dimension + j] = av;

                if ((*stopCurrent == 1)&&(diff > args->precision)) {
                    pthread_mutex_lock(&lock);
                    *stopCurrent = 0;
                    pthread_mutex_unlock(&lock);
                }

            }
        }
        test++;


        pthread_barrier_wait(&barrier);

        if (*stopCurrent == 1) {
            pthread_exit(0);
            break;
        }
        *stopNext = 1;
        pthread_barrier_wait(&barrier);


        if (count == 0){
            localOne = args->one;
            localTwo  = args->two;
            stopCurrent = args->stopOne;
            stopNext = args->stopTwo;
            count += 1;

        }

        else{
            localOne = args->two;
            localTwo  = args->one;
            stopCurrent = args->stopTwo;
            stopNext = args->stopOne;
            count -= 1;
        }

    }

}

double *checkPrecision(double *arrayOne, double*arrayTwo, int dim, double precision){
    double *arrayOut = malloc(sizeof(double)*dim*dim);
    int i;
    double diff;
//    printf("Precision = %f\n", precision);
    for (i=0; i<dim*dim; i++){
        diff = abs(arrayOne[i] - arrayTwo[i]);
//        printf("Check %d = %f = %f - %f\n", i, diff, arrayOne[i], arrayTwo[i]);
        arrayOut[i] = diff<precision;
    }
    return arrayOut;
}

pthread_t  *createThread(int numThreads) {
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
    return thread;
}

void* solverSeq(void *in) {
    struct arg_struct *args = (struct arg_struct *) in;

    double a, b, c, d, av;
    double * current;
    int test = 0;
    bool stop = false;

    while (!stop) {
        stop = true;
        test++;
        for (int i = 1; i < args->dimension-1; i++) {
            for (int j = 1; j < args->dimension-1; j++) {
                current = ((args->two+i*args->dimension) + j);
                a = *((args->one+(i-1)*args->dimension) + j);
                b = *((args->one+i*args->dimension) + (j-1));
                c = *((args->one+(i+1)*args->dimension) + j);
                d = *((args->one+i*args->dimension) + (j+1));
                av = average(a, b, c, d);
                if (fabs(av - *current) > args->precision) {
                    stop = false;
                }
                args->two[i*args->dimension + j] = av;

            }
        }
        double *temp = args->one;
        args->one = args->two;
        args->two = temp;

    }
    printf("test:%d\n", test);

}

double *doSolve(struct arg_struct args,  int numThreads){
    if (numThreads ==1) {
        solverSeq(&args);
        return args.one;
    }
    else {
        pthread_t *thread = createThread(numThreads);
        int i;
        for (i = 0; i < numThreads; i++) {
            if (pthread_create(&thread[i], NULL, solver, (void *) &args) != 0) {
                printf("Error. \n");
                exit(EXIT_FAILURE);
            }
        }
        for (i = 0; i < numThreads; i++) {
            pthread_join(thread[i], NULL);
        }
        return args.one;
    }

}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        unsigned int numThreads = (unsigned int)atoi(argv[1]);
        int dimension = atoi(argv[2]);
        double precision;

        precision = atof(argv[3]);
        int arraySize = dimension*dimension;



        int i;


        double arr1[arraySize];
        double arr2[arraySize];


        FILE * fp = fopen("./numbers.txt", "r+");
        setArray(arr1, arr2, arraySize, fp);
        pthread_t *thread = createThread(numThreads);
        pthread_t *sThread = createThread(1);


        struct arg_struct argsSeq;
        argsSeq.dimension = dimension;
        argsSeq.precision = precision;
        argsSeq.one = arr1;
        argsSeq.two = arr2;
        argsSeq.numThreads = 1;
        argsSeq.stopOne = malloc(sizeof(int));
        argsSeq.stopTwo = malloc(sizeof(int));

        struct arg_struct args;
        args.dimension = dimension;
        args.precision = precision;
        args.one = arr1;
        args.two = arr2;
        args.numThreads = numThreads;
        args.stopOne = malloc(sizeof(int));
        args.stopTwo = malloc(sizeof(int));

        struct timespec begin, end;
        long time_spent;


        clock_gettime(CLOCK_MONOTONIC, &begin);
        double *par = doSolve(args, args.numThreads);
        clock_gettime(CLOCK_MONOTONIC, &end);
        time_spent = (1000000000L * (end.tv_sec - begin.tv_sec)) + end.tv_nsec - begin.tv_nsec;
        printf("cores: %d, array size: %d, time: %llu nanoseconds.\n", numThreads, args.dimension, (unsigned long long int) time_spent);
//        printf("Parallel Result: \n");
//        printArray(par, dimension);

//        clock_gettime(CLOCK_MONOTONIC, &begin);
//        double *seq = doSolve(argsSeq, 1);
//        clock_gettime(CLOCK_MONOTONIC, &end);
//        time_spent = (1000000000L * (end.tv_sec - begin.tv_sec)) + end.tv_nsec - begin.tv_nsec;
//        printf("cores: %d, array size: %d, time: %llu nanoseconds.\n", 1, args.dimension, (unsigned long long int) time_spent);
//        printf("Sequential Result: \n");
//        printArray(seq, dimension);


        //38440253
//        printf("Sequential Result: \n");
//        printArray(seq, dimension);

//        double *check = malloc(sizeof(double)*dimension*dimension);
//        check = checkPrecision(seq, par, dimension, precision);
////        printf("Check Correctness: a value of 1 indicates\ndifference is within precision\n");
//        printArray(check, dimension);
//        int sum = 0;
//        for (i = 0; i<dimension*dimension; i++){
//            sum += check[i];
//        }
//        printf("Sum of Correctness array should be dim^2 = %d\nSum: %d", dimension*dimension, sum);

        pthread_mutex_destroy(&lock);
    }
}