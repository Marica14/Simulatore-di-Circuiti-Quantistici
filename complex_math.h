#ifndef COMPLEX_MATH_H 
#define COMPLEX_MATH_H

//Definizione dei numeri complessi come una struct,dato che z = a+ib, dove a, b sono numeri reali e i = radice di(−1) 
//(e quindi i^2 = −1).

typedef struct complex_struct{
	double a; //a
	double ib; //b * rad(-1)
} complx_t; //aggiungo _t alla fine del nome per indicare che e un tipo di dato definito dall'utente


//Firma delle funzioni per le operazioni sui numeri complessi
//Dati z = a + ib e w = x + iy:


//Funzione per la somma: z + w = (a + x) + i(b + y);
complx_t complex_sum(complx_t z, complx_t w);

//Funzione per il prodotto: zw = (ax − by) + i(ay + bx);
complx_t complex_prd(complx_t z, complx_t w);

//Funzione per il modulo: |z| = radice di(a^2 + b^2)
double complex_mod(complx_t z);

//Funzione per il quadrato del modulo: |z|^2 = (radice di(a^2 + b^2))^2 ovvero |z|^2 = a^2 + b^2
double complex_mod_sq(complx_t z);

#endif
