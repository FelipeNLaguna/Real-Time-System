#include "calculus.h"
#include <stdio.h>

void write_file(const float t, const float y[3], const float u[2]){
    fprintf(file, "%g\t", t);
    fprintf(file, "%g\t%g\t", u[0], u[1]);
    fprintf(file, "%g\t%g\t%g\n", y[0], y[1], y[2]);
}