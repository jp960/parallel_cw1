#include <stdio.h>
#include <math.h>
#include <pthread.h>

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

void solver(int dimension, double inputArray[], double copy[], int numThreads, double precision) {
    printf("Dimension: %d\n", dimension);
    double a, b, c, d, av;
    double  current;
    int test = 0;
    int stop = 0;
    while (stop == 0) {
        test++;
        printf("round: %d\n", test);
        stop = 1;
        for (int i = 1; i < dimension-1; i++) {
            for (int j = 1; j < dimension-1; j++) {
                printf("%d, %d\n", i, j);
                current = *((inputArray+i*dimension) + j);
                printf("current: %lf\n", current);
                a = *((inputArray+(i-1)*dimension) + j);
                b = *((inputArray+i*dimension) + (j-1));
                c = *((inputArray+(i+1)*dimension) + j);
                d = *((inputArray+i*dimension) + (j+1));
                av = average(a, b, c, d);
                printf("average: %lf\n", av);
                printf("difference: %lf\n", fabs(av - current));
                if (fabs(av - current) > precision) {
                    inputArray[i*dimension + j] = av;
                    stop = 0;
                }
            }
        }
        printf("\n");
        printArray(inputArray, dimension);
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    double arr[25] = {5, 7, 8, 9, 11, 2, 2, 7, 5, 7, 9, 6, 1, 8, 2, 8, 3, 9, 4, 3, 3, 4, 7, 8, 9};
    double arr2[25] = {5, 7, 8, 9, 11, 2, 2, 7, 5, 7, 9, 6, 1, 8, 2, 8, 3, 9, 4, 3, 3, 4, 7, 8, 9};
    solver(5, arr, arr2, 1, 0.0001);
    return 0;
}
