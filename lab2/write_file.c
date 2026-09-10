#include "calculus.h"
#include <stdio.h>

void write_file(float t, float y[3][1], float u[2][1]){
    fprintf(file, "%.2f\t", t);
    fprintf(file, "%.2f\t %.2f\t", u[0][0], u[1][0]);
    fprintf(file, "%.2f\t%.2f\t%.2f\n", y[0][0], y[1][0], y[2][0]);
}