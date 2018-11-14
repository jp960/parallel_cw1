#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <pthread.h>

void solver(int dimension, double *inputArray, int numThreads, double precision);
double average (double a, double b, double c, double d);

int main() {
//    double arr[5][5] = {{5, 7, 8, 9, 11}, {2, 2, 7, 5, 7}, {9, 6, 1, 8, 2}, {8, 3, 9, 4, 3}, {3, 4, 7, 8, 9}};
    double arr[25] = {5, 7, 8, 9, 11, 2, 2, 7, 5, 7, 9, 6, 1, 8, 2, 8, 3, 9, 4, 3, 3, 4, 7, 8, 9};
    solver(5, (double *)arr, 1, 0.0001);
    return 0;
}

void solver(int dimension, double *inputArray, int numThreads, double precision) {
    printf("%d\n", dimension);
    double a, b, c, d, av;
    double * current;
    int test = 0;
    bool stop = false;
    while (!stop) {
        test++;
        printf("round: %d\n", test);
        stop = true;
        for (int i = 1; i < dimension-1; i++) {
            for (int j = 1; j < dimension-1; j++) {
                printf("%d, %d\n", i, j);
                current = ((inputArray+i*dimension) + j);
                printf("%lf\n", *current);
                a = *((inputArray+(i-1)*dimension) + j);
                b = *((inputArray+i*dimension) + (j-1));
                c = *((inputArray+(i+1)*dimension) + j);
                d = *((inputArray+i*dimension) + (j+1));
                av = average(a, b, c, d);
                printf("%lf\n", av);
                printf("%lf\n", fabs(av - *current));
                if (fabs(av - *current) > precision) {
                    *current = av;
                    stop = false;
                }
                printf("\n");
            }
        }
    }

}

double average (double a, double b, double c, double d) {
    return (a+b+c+d)/4.0;
}