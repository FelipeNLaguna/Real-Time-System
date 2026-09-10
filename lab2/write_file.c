#include "calculus.h"
#include <stdio.h>

void write_file(const float t, const float y[3], const float u[2]){
    fprintf(file, "%f\t", t);
    fprintf(file, "%f\t%f\t", u[0], u[1]);
    fprintf(file, "%f\t%f\t%f\n", y[0], y[1], y[2]);
}