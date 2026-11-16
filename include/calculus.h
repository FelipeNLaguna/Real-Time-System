#ifndef CALCULUS_H
#define CALCULUS_H

#include <stdio.h>

extern void derivate(const double u[2], const double x[3], double x_der[3]);
extern void result_y(const double x[3], double y[3]);
extern void next_x(double x[3], const double pre_x[3], const double x_der[3], const double dt); 


#endif
