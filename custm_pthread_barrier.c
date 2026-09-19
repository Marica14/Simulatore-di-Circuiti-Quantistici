#include "custm_pthread_barrier.h"

//Funzioni per la gestione della barriera

//Inizializza la barriera con il numero di thread richiesti
void custm_pthread_barrier_init(custm_pthread_barrier_t *b, int count) {
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->cond, NULL);
    b->threads_required = count;
    b->count = count;
    b->cycle = 0;
}

//Funzione per far attendere i thread alla barriera finché tutti non arrivano
void custm_pthread_barrier_wait(custm_pthread_barrier_t *b) {
    pthread_mutex_lock(&b->mutex);
    int current_cycle = b->cycle;
    b->count--;

    if (b->count == 0) {
        b->cycle++; //Aggiornamento del ciclo: evita che un thread più veloce rientri subito in un nuovo giro di barriera prima che tutti gli altri abbiano lasciato quello precedente
        b->count = b->threads_required; //Reset del contatore per il prossimo utilizzo
        pthread_cond_broadcast(&b->cond); //Risvegliamento di tutti gli altri thread
    } else {
        //Altrimenti in attesa finché il ciclo non cambia
        while (current_cycle == b->cycle) {
            pthread_cond_wait(&b->cond, &b->mutex);
        }
    }
    pthread_mutex_unlock(&b->mutex);
}

//Distrugge la barriera e libera le risorse associate
void custm_pthread_barrier_destroy(custm_pthread_barrier_t *b) {
    pthread_mutex_destroy(&b->mutex);
    pthread_cond_destroy(&b->cond);
}
