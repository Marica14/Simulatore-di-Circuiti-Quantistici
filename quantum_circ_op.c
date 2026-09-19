#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "quantum_circ_op.h"
#include <time.h>


//Evoluzione unitaria multi-thread: ogni thread calcola una porzione del nuovo stato


/*
Funzione eseguita da ogni thread: ripercorre l'intero circuito (tutte le porte in sequenza),
ma per ciascuna porta calcola solo la propria porzione di righe del prodotto matrice-vettore.

Le due barriere ad ogni iterazione servono per sincronizzare: la prima assicura che tutti i thread
abbiano finito i calcoli sulla porta corrente prima di procedere, la seconda assicura che lo scambio
dei puntatori (fatto dal solo thread 0) sia completato prima che gli altri thread rileggano v_in.
*/
void* matrix_mult_thread(void* arg) {
    thread_data_t *data = (thread_data_t*)arg;

    //Il thread esegue l'intero circuito
    for (int op = 0; op < data->tot_circ_gates; op++) {
        
        // Accesso immediato O(1) alla matrice corrente senza usare stringhe
        complx_t *current_matrix = data->ops_matrices[op];

        //Estrazione dei puntatori correnti per questa iterazione
        complx_t *v_in = *(data->v_in);
        complx_t *v_out = *(data->v_out);

        //Calcolo della propria porzione di matrice
        for (int i = data->start_row; i < data->end_row; i++) {
            complx_t sum_val = {0.0, 0.0};
            for (int j = 0; j < data->nstates; j++) {
                int matrix_idx = i * data->nstates + j;
                sum_val = complex_sum(sum_val, complex_prd(current_matrix[matrix_idx], v_in[j]));
            }
            v_out[i] = sum_val;
        }

        //Barriera 1: attende che tutti i thread abbiano finito i calcoli
        custm_pthread_barrier_wait(data->barrier);

        //Solo il thread 0 inverte i puntatori per l'iterazione successiva
        //Necessaria altrimenti gli altri thread potrebbero leggere v_in prima dello swap.
        if (data->thread_id == 0) {
            complx_t *temp = *(data->v_in);
            *(data->v_in) = *(data->v_out);
            *(data->v_out) = temp;
        }

        //Barriera 2: Attende che il thread 0 abbia finito l'inversione prima di procedere
        custm_pthread_barrier_wait(data->barrier);
    }
    
    pthread_exit(NULL);
}



//Funzione principale che esegue l'evoluzione unitaria multi-thread
void simulate_circuit(int nthreads, int nstates, complx_t **state, circ_t *circuit, int num_circ_ops, gate_t *gates, int num_gates) {
    
    //Vettore temporaneo per salvare l'output della porta corrente
    complx_t *temp_v_out = (complx_t*)malloc(nstates * sizeof(complx_t));
    if (temp_v_out == NULL) {
        perror("Error allocating memory for the temporary vector");
        exit(EXIT_FAILURE);
    }

    //Array per memorizzare i puntatori diretti alle matrici richieste dal circuito
    complx_t **ops_matrices = (complx_t**)malloc(num_circ_ops * sizeof(complx_t*));
    if (ops_matrices == NULL) {
        perror("Error allocating memory for operations array");
        exit(EXIT_FAILURE);
    }

    //Il thread principale esegue un'unica volta le strcmp
    for (int op = 0; op < num_circ_ops; op++) {
        int found = 0;
        for (int g = 0; g < num_gates; g++) {
            if (strcmp(gates[g].name, circuit[op].gate_name) == 0) {
                ops_matrices[op] = gates[g].matrix;
                found = 1;
                break;
            }
        }
        
        if (!found) {
            fprintf(stderr, "Errore: porta '%s' non definita\n", circuit[op].gate_name);
            exit(EXIT_FAILURE);
        }
    }

    //Creazione dei thread e inizializzazione della barriera
    pthread_t threads[nthreads];
    thread_data_t t_data[nthreads];
    custm_pthread_barrier_t barrier;
    custm_pthread_barrier_init(&barrier, nthreads);

    int rows_per_thread = nstates / nthreads;
    int remaining_rows = nstates % nthreads; 
    int curr_row = 0;

    for (int t = 0; t < nthreads; t++) {
        t_data[t].thread_id = t;
        t_data[t].start_row = curr_row;
        int extra = (t < remaining_rows) ? 1 : 0;
        t_data[t].end_row = curr_row + rows_per_thread + extra;
        t_data[t].nstates = nstates;
        
        t_data[t].tot_circ_gates = num_circ_ops;
        t_data[t].circuit = circuit;
        t_data[t].gates = gates;
        t_data[t].tot_gates = num_gates;
        t_data[t].ops_matrices = ops_matrices;
        
        t_data[t].v_in = state; 
        t_data[t].v_out = &temp_v_out;
        t_data[t].barrier = &barrier;

        curr_row = t_data[t].end_row;

        if (pthread_create(&threads[t], NULL, matrix_mult_thread, (void*)&t_data[t]) != 0) {
            perror("Error in thread creation");
            exit(EXIT_FAILURE);
        }
    }

    //Attesa della terminazione di tutti i thread
    for (int t = 0; t < nthreads; t++) {
        pthread_join(threads[t], NULL);
    }


    /*
    Gestione della memoria finale: a seconda della parità del numero di porte applicate,
    il puntatore *state e temp_v_out possono aver scambiato ruolo un numero dispari di volte.
    Se il numero di porte è dispari, il risultato finale si trova in temp_v_out: va quindi
    copiato nell'area di memoria originale di *state (o il puntatore va aggiornato) e la vecchia
    area va liberata. 
    Se è pari, i puntatori sono già tornati alle posizioni di partenza.
    */
    if (num_circ_ops % 2 != 0) {
        //Vengono copiati i risultati finali nell'area di memoria corretta (quella del main)
        for(int i = 0; i < nstates; i++) {
            temp_v_out[i] = (*state)[i];
        }
        //Liberazione della memoria temporanea (che attualmente è puntata da *state)
        free(*state);
        //Ripristino del puntatore allo stato originale in modo che il main possa usarlo
        *state = temp_v_out;
    } else {
        //Se pari,i puntatori sono tornati alle posizioni di partenza
        free(temp_v_out);
    }

    //Distruzione della barriera custom
    custm_pthread_barrier_destroy(&barrier);

    //Liberazione dell'array di puntatori alle matrici (non più necessario)
    free(ops_matrices);
}

//Funzione per la misurazione dello stato finale: campiona lo stato finale e stampa le probabilità stimate per ogni stato
void measure_state(complx_t *state, int nstates, int num_measurements) {
    //Array per tenere traccia di quante volte esce ogni stato
    int *counts = (int*)calloc(nstates, sizeof(int));

    if (counts == NULL) {
        perror("Error allocating memory for the counts array");
        exit(EXIT_FAILURE);
    }
    
    //Array per la CDF (Cumulative Distribution Function)
    double *cdf = (double*)malloc(nstates * sizeof(double));

    if (cdf == NULL) {
        perror("Error allocating memory for the CDF");
        exit(EXIT_FAILURE);
    }
    
    //Costruzione della CDF
    double cumulative_prob = 0.0;
    for (int i = 0; i < nstates; i++) {
        cumulative_prob += complex_mod_sq(state[i]);
        cdf[i] = cumulative_prob;
    }

    /*
    Inizializzazione del generatore di numeri casuali usando il tempo attuale
    Si preferisce /dev/urandom a time(NULL) perché fornisce numeri casuali di qualità migliore
    (evita che esecuzioni ravvicinate nello stesso secondo producano lo stesso seed)
    */
   
    unsigned int seed; 
    FILE *urandom = fopen("/dev/urandom", "r");
    if (urandom != NULL) {
        fread(&seed, sizeof(unsigned int), 1, urandom);
        fclose(urandom);
    } else {
        seed = (unsigned int)time(NULL); // Fallback
    }
    srand(seed);

    //Esecuzione delle misurazioni richieste
    for (int m = 0; m < num_measurements; m++) {
        double r = (double)rand() / RAND_MAX;
        
        //Trova in quale "scaglione" della CDF cade il numero
        for (int i = 0; i < nstates; i++) {
            if (r <= cdf[i]) {
                counts[i]++; 
                break;    
            }
        }
    }

    //Calcolo di quanti qubit ci sono per stampare in binario
    int nqubits = 0;
    int temp = nstates;
    while (temp > 1) {
        temp >>= 1;
        nqubits++;
    }

    //Stampa dei risultati stimati: vengono stampati solo gli stati che sono usciti almeno una volta
    for (int i = 0; i < nstates; i++) {
        if (counts[i] > 0) {
            double estimated_prob = (double)counts[i] / num_measurements;
            
            for (int b = nqubits - 1; b >= 0; b--) {
                int bit = (i >> b) & 1;
                printf("%d", bit);
            }
            
            //Formattazione richiesta
            printf(" @ %.2f\n", estimated_prob);
        }
    }

    //Pulizia della memoria
    free(counts);
    free(cdf);


}