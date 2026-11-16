#include <math.h>
#include "calculus.h"
#define D 0.6

/*
int matrix_y[3][3] = {
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1}
};
*/

void result_y(const double x[3], double y[3]){
   
    double seno_matriz = sin(x[2]);
    double cos_matriz = cos(x[2]);

    y[0] = x[0] + (0.5 * D * cos_matriz);
    y[1] = x[1] + (0.5 * D * seno_matriz);
    y[2] = x[2];

}