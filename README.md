# Simulatore di Circuiti Quantistici


## Descrizione del Progeto
Questo progetto è un programma C multi-thread progettato per simulare semplici circuiti quantistici. 
La simulazione è divisa in due fasi:
1.Evoluzione unitaria: lo stato del circuito evolve secondo l'applicazione di matrici (porte quantistiche).Il calcolo del prodotto matrice-vettore è implementato in modalità multi-thread.
2.Misurazione: se richiesta, stima la distribuzione di probabilità dell'uscita del circuito tramite campionamento.


## File inclusi
Il codice è stato modularizzato per massimizzare la chiarezza dell'architettura:

### main.c: 
Contiene il main del programma.Gestisce il parsing degli argomenti da linea di comando,la lettura e interpretazione dei file di input (#qubits, #init, #define, #circ) e la stampa dell'output finale secondo il formato richiesto. 

### quantum_circ_op.c : 
Contiene la logica core della simulazione.Implementa l'header quantum_circ_op.h con la funzione 'matrix_mult_thread' eseguita dai thread e la gestione del ciclo del circuito, oltre alla funzione di
misurazione probabilistica basata su CDF.

### custm_pthread_barrier.c:
Implementa una barriera custom (basata su mutex e variabile di condizione) usata per sincronizzare i thread dopo ogni porta del circuito. È un componente generico e riusabile, indipendente dalla logica della simulazione quantistica.

### complex_math.c:
Libreria matematica che implementa le funzioni dell'header complex_math.h per la gestione delle operazioni sui numeri complessi (somma, prodotto, modulo,modulo quadro) richieste dal simulatore.

### Makefile:
Script per la compilazione automatica del progetto.


## Manuale utente

### Compilazione
Per compilare il progetto, posizionarsi nella cartella contenente i sorgenti e digitare il seguente comando sul terminale: make

Questo genererà l'eseguibile chiamato 'main' (oltre ai file oggetto). 

Per ripulire la cartella dai file compilati, eseguire: 'make clean'.



## Esecuzione da linea di comando
I dati di ingresso sono specificati dall'utente nella linea di comando. 
È obbligatorio specificare il numero di thread tramite l'opzione '-t' e specificare il file con la quantità di qubits prima del file con la definizione delle matrici.

### Sintassi:
./main  <input_circuito.txt> <struttura_circuito.txt> -t <numero_thread>

### Esempio d'uso:
./main input_circuito.txt struttura_circuito.txt -t 4

### Formato di output
Il programma stampa i risultati sullo standard output (stdout).
Se il file di input NON contiene la direttiva 'measure': stamperà lo 
stato finale (vettore di numeri complessi) nello stesso formato dello stato iniziale.

Se il file di input contiene la direttiva 'measure' stamperà la distribuzione di probabilità stimata per i vari stati, indicando la codifica binaria e la probabilità (es. 01 @ 0.40).
