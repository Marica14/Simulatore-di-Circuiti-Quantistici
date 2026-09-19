#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "complex_math.h"
#include <unistd.h>
#include <getopt.h>
#include "quantum_circ_op.h"


/*
Funzione che funge da parser per i numeri complessi e ignora in automatico parentesi e virgole (es: "[(0, 1)" estrae 0, poi estrae 1)
Prende in input un file pointer e un puntatore a complx_t dove salvare il numero complesso letto
Ritorna 1 se la lettura del numero complesso ha successo, 0 in caso di errore o fine file
*/
int read_next_complex(FILE *fl, complx_t *ncomplex) {
    double a = 0.0, ib = 0.0;
    char chr;

    //Salta tutti i caratteri che non sono numeri o segni
    while (fscanf(fl, " %c", &chr) == 1) {
        if ((chr >= '0' && chr <= '9') || chr == '+' || chr == '-' || chr == '.') {
            ungetc(chr, fl); // Trovato l'inizio di un numero, viene rimesso nel file stream per essere letto correttamente da fscanf
            break;
        }
    }

    //Lettura della parte reale
    if (fscanf(fl, "%lf", &a) != 1) return 0; // Se fallisce, ritorna errore

    //Controllo della presenza di una parte immaginaria che inizia con + o -
    if (fscanf(fl, " %c", &chr) == 1) {
        if (chr == '+' || chr == '-') {
            char sign = chr; //Salvataggio del segno
            char i_char;
            
            //Lettura del carattere successivo, che dovrebbe essere la 'i'
            if (fscanf(fl, " %c", &i_char) == 1 && i_char == 'i') {
                if (fscanf(fl, "%lf", &ib) == 1) {
                    if (sign == '-') {
                        ib = -ib;
                    }
                }
            } else {
                ungetc(i_char, fl);
                ungetc(sign, fl);
            }
        } else {
            ungetc(chr, fl); //Non era un segno, viene rimesso nel file stream
        }
    }

    ncomplex->a = a;
    ncomplex->ib = ib;
    return 1;
}

int main(int argc, char* argv[]){
    //Inizializzazione dei dati necessari per la simulazione del circuito quantistico
	int nthreads = 0;
	int option;
	int nqubits = -1;
    int nstates = 0;
    complx_t *init_state = NULL;

    //Inizializzazione delle strutture dati per le porte e il circuito
	gate_t *gates = NULL;
    int ngates = 0;


    circ_t *circuit = NULL;
    int ncirc_ops = 0;
    int nmeasurement = 0;

	//Lettura della riga di comando e ricerca dell'opzione -t che indica il numero di thread da usare, se la trova la salva nella variabile nthreads
	while ((option = getopt(argc,argv, "t:"))!= -1){
		switch(option){
			case 't':
				nthreads = atoi(optarg);
				break;
			default: 
                //Gestione errore, valore di -t non valido (es. una stringa)) 
				fprintf(stderr, "For example: %s <file_input1.txt> <file_input2.txt>  -t <nthreads>\n", argv[0]);
                exit(EXIT_FAILURE);
		}
			
	}

	
	//Gestione errore,l'utente ha inserito un numero di thread valido <= 0
	if (nthreads <= 0) {
       fprintf(stderr, "Error: You must specify a valid number of threads (>0) with the -t option.\n");
       fprintf(stderr, "Example: %s <file_input1.txt> <file_input2.txt>  -t <nthreads>\n", argv[0]);
       exit(EXIT_FAILURE);
    }

	 
	//Parser per la lettura dei file passati come argomenti
	for (int i = optind; i < argc; i++) {	         
	    FILE *in_file = fopen(argv[i], "r");
	    if (in_file == NULL) {
	       perror("Error opening file, invalid input file");
	       exit(EXIT_FAILURE);
	    }

        //Assegnazione di 1MB di buffer locale al file pointer
        //Per ridurre le chiamate di sistema (read) su disco durante la fscanf
        char buff[1048576];
        setvbuf(in_file, buff, _IOFBF, sizeof(buff));
	 
	    char temp[100]; 
	 
	    /*Parser principale a token: 
        legge una parola alla volta dal file e la interpreta come direttiva (#qubits, #init, #define, #circ).
        Ogni ramo consuma i dati associati alla propria direttiva prima di tornare a leggere il token successivo*/
	    while (fscanf(in_file, "%99s", temp) == 1) {
	    	// Controllo della direttiva appena letta

			//#qubits
	     	if (strcmp(temp, "#qubits") == 0) {
				 
	            if (fscanf(in_file, "%d", &nqubits) == 1) {
					nstates = 1 << nqubits; //Calcolo del numero di stati possibili (2^nqubits)
                    
                    //Libera uno stato eventualmente già allocato da una precedente direttiva #qubits nello stesso file,
					//per evitare memory leak nel caso (raro ma possibile) in cui #qubits compaia più volte
					if (init_state != NULL) free(init_state);

					//Allocazione dinamica  dell'array per lo stato iniziale
					init_state = (complx_t*)malloc(nstates * sizeof(complx_t));
					if (init_state == NULL) {
						perror("Error allocating memory for initial state");
						exit(EXIT_FAILURE);
	                }
				}
			
            //#init
	        } else if (strcmp(temp, "#init") == 0) {

				if (init_state == NULL) {
                    fprintf(stderr, "Error: Found directive #init before #qubits. Make sure to specify #qubits before #init.\n");
                    exit(EXIT_FAILURE);
	            }
				

				char bracket;
				fscanf(in_file, " %c", &bracket);
				
				if (bracket != '[') {
					fprintf(stderr, "Error: expected '[' at the beginning of the initial state.\n");
					exit(EXIT_FAILURE);
				}

				
                for(int j = 0; j < nstates; j++){
                    if (!read_next_complex(in_file, &init_state[j])) {
                        fprintf(stderr, "Error reading the initial state at index %d\n", j);
                        exit(EXIT_FAILURE);
                    }
                }

                //Verifica della norma dello stato iniziale
                double norm = 0.0;
                for (int j = 0; j < nstates; j++) {
                    norm += complex_mod_sq(init_state[j]);
                }

                if (norm < 0.999 || norm >= 1.001) {
                    fprintf(stderr, "Error: The norm of the initial state is %0.2lf and not 1. Make sure the state is normalized.\n", norm);
                    exit(EXIT_FAILURE);
                }
			
			//#define
			} else if (strcmp(temp, "#define") == 0) {	            
				//Parsing delle matrici delle porte quantistiche
                char gate_name[100];
                fscanf(in_file, "%99s", gate_name); //Lettura del nome

                if (nstates == 0) {
                    fprintf(stderr, "Error: #define found before #qubits\n");
                    exit(EXIT_FAILURE);
                }

                //Espansione dell'array vuoto per dare spazio alla nuova porta
                ngates++;
                gate_t *temp_gates = realloc(gates, ngates * sizeof(gate_t));

                if (temp_gates == NULL) {
                    perror("Error with realloc gates");
                    free(gates); 
                    exit(EXIT_FAILURE);
                }
                gates = temp_gates;

                strcpy(gates[ngates - 1].name, gate_name);

                //La matrice è 2^n * 2^n, quindi nstates * nstates
                int matrix_size = nstates * nstates;
                gates[ngates - 1].matrix = (complx_t*)malloc(matrix_size * sizeof(complx_t));

                if (gates[ngates - 1].matrix == NULL) {
                    perror("Error allocating memory for gate matrix");
                    exit(EXIT_FAILURE);
                }


                //Lettura di tutti i numeri complessi della matrice
                for (int j = 0; j < matrix_size; j++) {
                    if (!read_next_complex(in_file, &gates[ngates - 1].matrix[j])) {
                        fprintf(stderr, "Error reading the matrix %s\n", gate_name);
                        exit(EXIT_FAILURE);
                    }
                }
			
			//#circ
	        }else if (strcmp(temp, "#circ") == 0) {                
                char gate_name[50];
                
                //Lettura della struttura del circuito fino a quando non viene trovato "measure" o un commento
                while (fscanf(in_file, "%49s", gate_name) == 1 && gate_name[0] != '#') {
                    
                    //Se viene trovato "measure" il prossimo numero è il numero di misurazioni
                    if (strcmp(gate_name, "measure") == 0) {
                        if (fscanf(in_file, "%d", &nmeasurement) != 1) {
                            fprintf(stderr, "Error: number of measurements not specified after 'measure'.\n");
                            exit(EXIT_FAILURE);
                        }
                        break; //Uscita dal ciclo perché non vogliamo leggere ulteriori porte dopo "measure"
                    } else {
                        //Altrimenti è il nome di una porta da aggiungere al circuito
                        ncirc_ops++;
                        circ_t *temp_circ = realloc(circuit, ncirc_ops * sizeof(circ_t));

                        if(temp_circ == NULL) {
                            perror("Error with realloc circuit");
                            free(circuit); 
                            exit(EXIT_FAILURE);
                        }
                        
                        circuit = temp_circ;
                        strcpy(temp_circ[ncirc_ops - 1].gate_name, gate_name);
                    }
                }
                
                //Se la parola letta è una nuova direttiva (perchè inizia con '#'), riavvolge il cursore del file per farla processare al ciclo principale.
                if (gate_name[0] == '#') {
                    //Riavvolgimento necessario perché fscanf ha già consumato il token della direttiva successiva
                    //(es. "#define" o "#circ"): senza questo fseek il ciclo principale la perderebbe e non la processerebbe
                    fseek(in_file, -strlen(gate_name), SEEK_CUR);
                }
	        }
	    }
        //Chiusura del file 
		fclose(in_file);
	}
	 
	
	simulate_circuit(nthreads, nstates, &init_state, circuit, ncirc_ops, gates, ngates);

    
    //Gestione dell'output finale: se è stata richiesta la misurazione, stampa i risultati della misurazione, altrimenti stampo lo stato finale
    if (nmeasurement > 0) {
        measure_state(init_state, nstates, nmeasurement);
    } else {
        printf("[ ");
        for (int j = 0; j < nstates; j++) {
            if (init_state[j].ib >= 0) {
                printf("%.5f+i%.5f", init_state[j].a, init_state[j].ib);
            } else {
                //Se la parte immaginaria è negativa
                printf("%.5f-i%.5f", init_state[j].a, -init_state[j].ib);
            }
            
            if (j < nstates - 1) {
                printf(",  ");
            }
        }
        printf(" ]\n");
    }


    //Deallocazione memoria a fine programma
    if (init_state != NULL) free(init_state);
    if (circuit != NULL) free(circuit);

    //Deallocazione di tutte le matrici e l'array delle porte
    for (int i = 0; i < ngates; i++) {
        if (gates[i].matrix != NULL) free(gates[i].matrix);
    }
    if (gates != NULL) free(gates);

    return 0; 
}
