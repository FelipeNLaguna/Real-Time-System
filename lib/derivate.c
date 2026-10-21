#include <math.h>
#include "calculus.h"

// calula seno e cos
// declara matriz
void derivate(const double u[2], const double x[3], double x_der[3]){

    double seno_matriz = sin(x[2]);
    double cos_matriz = cos(x[2]);

/*
    double matrix_x[3][2] = {
        {seno_matriz, 0},
        {cos_matriz, 0},
        {0, 1}
    };
*/

    x_der[0] = seno_matriz;
    x_der[1] = cos_matriz;
    x_der[2] = u[1];
}