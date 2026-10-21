#ifndef CALCULUS_H
#define CALCULUS_H

#include <stdio.h>

// vou declara como void ja que vamos resolver problemas em arrays
extern void derivate(const double u[2], const double x[3], double x_der[3]);
extern void result_y(const double x[3], double y[3]);
extern void next_x(double x[3], const double pre_x[3], const double x_der[3], const double dt); 

extern FILE *file;

#endif
