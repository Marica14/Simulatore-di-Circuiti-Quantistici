#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "complex_math.h"

//Dati z = a + ib e w = x + iy:

//Funzione per la somma: z + w = (a + x) + i(b + y);
complx_t complex_sum(complx_t z, complx_t w){
	complx_t result;
	result.a = z.a + w.a;
	result.ib = z.ib + w.ib;
	return result;
}

//Funzione per il prodotto: zw = (ax − by) + i(ay + bx);
complx_t complex_prd(complx_t z, complx_t w){
	complx_t result;
	result.a = ((z.a*w.a)-(z.ib * w.ib));
	result.ib = ((z.a*w.ib) + (w.a*z.ib));
	return result;
}

//Funzione per il modulo: |z| = radice di(a^2 + b^2)
double complex_mod(complx_t z){
	double result =sqrt( (z.a * z.a) + (z.ib * z.ib));
	return result;
}

//Funzione per il quadrato del modulo: |z|^2 = (radice di(a^2 + b^2))^2 ovvero |z|^2 = a^2 + b^2
double complex_mod_sq(complx_t z){
	double result = (z.a * z.a) + (z.ib * z.ib);
	return result;
}


