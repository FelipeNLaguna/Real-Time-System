#include "calculus.h"

/*
int matrix_y[3][3] = {
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1}
};
*/

void result_y(const float x[3], float y[3]){
    int i;

    for(i = 0; i < 3; i++){
       y[i] = x[i];
    }

}