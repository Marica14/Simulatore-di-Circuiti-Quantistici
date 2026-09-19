#ifndef CUSTM_PTHREAD_BARRIER_H
#define CUSTM_PTHREAD_BARRIER_H
#include <pthread.h>

//Barriera custom basata su mutex e variabile di condizione, riusabile in qualsiasi contesto multi-thread (non contiene nulla di specifico della simulazione quantistica)
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;             //Quanti thread sono arrivati
    int threads_required;  //Quanti thread bisogna aspettare in totale
    int cycle;             //Generazione della barriera (per evitare risvegli spuri)
} custm_pthread_barrier_t;

//Inizializzazione della barriera con il numero di thread richiesti
void custm_pthread_barrier_init(custm_pthread_barrier_t *barrier, int threads_required);

//Funzione per far attendere i thread alla barriera finché tutti non arrivano
void custm_pthread_barrier_wait(custm_pthread_barrier_t *barrier);

//Distrugge la barriera e libera le risorse associate
void custm_pthread_barrier_destroy(custm_pthread_barrier_t *barrier);

#endif
