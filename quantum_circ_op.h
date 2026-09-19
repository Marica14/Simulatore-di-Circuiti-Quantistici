#ifndef QUANTUM_CIRC_OP_H
#define QUANTUM_CIRC_OP_H
#include <pthread.h>
#include "complex_math.h"
#include "custm_pthread_barrier.h"


//Struct per memorizzare le matrici (Porte Quantistiche)
typedef struct {
    char name[50];
    complx_t *matrix; //Array 1D che conterrà la matrice (nstates * nstates)
} gate_t;

//Struct per memorizzare la sequenza di porte del circuito
typedef struct {
    char gate_name[50];
} circ_t;


//Struct per memorizzare i dati da passare a ogni singolo thread per la moltiplicazione
typedef struct {
    int thread_id;         //ID del thread
    int start_row;         //Riga di partenza (inclusa)
    int end_row;           //Riga di fine (esclusa)
    int nstates;           //Numero totale di stati (2^n)
    int tot_gates;         //Numero totale di porte definite
    int tot_circ_gates;    //Numero totale di porte nel circuito
    circ_t *circuit;       //Circuito da eseguire
    gate_t *gates;         //Array di tutte le porte definite
    complx_t **v_in;       //Puntatore al vettore di input (stato corrente).Doppio puntatore per permettere a tutti i thread di vedere lo scambio dei vettori fatto dal thread 0 dopo ogni porta
    complx_t **v_out;      //Puntatore al vettore di output (stato successivo), stesso motivo di v_in
    complx_t **ops_matrices; //Array di puntatori alle matrici delle porte usate nel circuito (accesso O(1) per indice). A differenza di v_in/v_out, qui il doppio puntatore serve solo per rappresentare un array di matrici, non per uno swap condiviso tra thread
    custm_pthread_barrier_t *barrier; //Barriera per sincronizzare i thread dopo ogni porta
} thread_data_t;



//Firma delle funzioni

// Funzione eseguita dai singoli thread per calcolare una porzione del nuovo stato
void* matrix_mult_thread(void* arg);

//Funzione principale che esegue l'evoluzione unitaria multi-thread
//Applica tutte le porte del circuito aggiornando lo stato
void simulate_circuit(int nthreads, int nstates, complx_t **state, circ_t *circuit, int num_circ_ops, gate_t *gates, int num_gates);

//Funzione per campionare lo stato finale se è richiesta la misurazione
void measure_state(complx_t *state, int nstates, int num_measurements);

#endif