CC = gcc 
CFLAGS = -g -Wall -Wextra -pthread 

#target  principale
main: main.o complex_math.o quantum_circ_op.o custm_pthread_barrier.o
	$(CC) $(CFLAGS) -o main main.o complex_math.o quantum_circ_op.o custm_pthread_barrier.o -lm


#regola per il main
main.o: main.c complex_math.h quantum_circ_op.h
	$(CC) $(CFLAGS) -c main.c

#regola per la matematica dei numeri complessi
complex_math.o: complex_math.c complex_math.h
	$(CC) $(CFLAGS) -c complex_math.c

#regola per le operazioni sui circuiti quantistici
quantum_circ_op.o: quantum_circ_op.c quantum_circ_op.h complex_math.h custm_pthread_barrier.h
	$(CC) $(CFLAGS) -c quantum_circ_op.c
	
#regola per la barriera custom di sincronizzazione tra thread
custm_pthread_barrier.o: custm_pthread_barrier.c custm_pthread_barrier.h
	$(CC) $(CFLAGS) -c custm_pthread_barrier.c

#regola di pulizia
.PHONY: clean
clean:
	rm -f *.o main
