#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// biblioteca de memoria compartilhada
#include <sys/shm.h>

#include "semun.h"
#include "estruturaCompartilhada.h"

int criarMemoriaCompartilhada();
void *associarEspacoMemoAoProcesso(int shmid);

void inicializarSemaforoEscritor(struct shared_memo *bufferCompartilhado);
void inicializarSemaforoLeitor(struct shared_memo *bufferCompartilhado);


int semaphore_p(int sem_id);
int semaphore_v(int sem_id);

int set_semvalue(int sem_id, int somar);
int get_semvalue(int sem_id);
void del_semvalue(int sem_id);

void terminarPrograma(int shmid, void *memoriaCompartilhada, struct shared_memo* bufferCompartilhado);

int main() {
		
		void *memoriaCompartilhada = (void *)0;
		struct shared_memo *bufferCompartilhado;
		int shmid;
		
		shmid = criarMemoriaCompartilhada();
		memoriaCompartilhada = associarEspacoMemoAoProcesso(shmid);
		
		printf("Memory attached at %X\n", (int)memoriaCompartilhada);
		
		bufferCompartilhado = (struct shared_memo *) memoriaCompartilhada;
		
		inicializarSemaforoEscritor(bufferCompartilhado);
		inicializarSemaforoLeitor(bufferCompartilhado);
		
		while(1) {
				
				// põe semaforo leitor ocupado
				if(!semaphore_p(bufferCompartilhado->sem_id_reader)) exit(EXIT_FAILURE) ;
				
				// adiciona mais um leitor ao contador
				bufferCompartilhado->qtdLeitores += 1;
				
				if (bufferCompartilhado->qtdLeitores == 1) semaphore_p(bufferCompartilhado->sem_id_writer);
				
				// libera semaforo de leitura
				semaphore_v(bufferCompartilhado->sem_id_reader);
				// lê dados
				printf("Lido: %s\n", bufferCompartilhado->texto);
				// ocupa leitor
				if(!semaphore_p(bufferCompartilhado->sem_id_reader)) exit(EXIT_FAILURE) ;
				
				// decrementa quantidade
				bufferCompartilhado->qtdLeitores -= 1;
				
				printf("Bloqueando...\n");
				sleep(3);
				// libera área de dados se leitores for 0
				if (bufferCompartilhado->qtdLeitores == 0) {
					semaphore_v(bufferCompartilhado->sem_id_writer);
					printf("Leitor liberado\n");
				}
				
				semaphore_v(bufferCompartilhado->sem_id_reader);
				
				sleep(rand() % 2);
		}
	terminarPrograma(shmid, memoriaCompartilhada, bufferCompartilhado);
	return 0;
}

// cria a memoria compartilhada e retorna o id
int criarMemoriaCompartilhada() {
	int sharedMemoId = shmget((key_t)1234, sizeof(struct shared_memo), 0666 | IPC_CREAT);
	
		// se id for -1 houve erro ao criar o espaço de memória compartilhada.
	if (sharedMemoId == -1) {
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
	}
	return sharedMemoId;
}

// faz o espaço ser acessível ao processo e retorna endereço para o primeiro byte
void *associarEspacoMemoAoProcesso(int shmid) {
	void *enderecoMemo = shmat(shmid, (void *)0, 0);
	
	if (enderecoMemo == (void *)-1) {
      fprintf(stderr, "shmat failed\n");
      exit(EXIT_FAILURE);
    }
    return enderecoMemo;
}

// desassocia memoria compartilhada do processo e deleta
void deletarMemoriaCompartilhada(int shmid, void *memoria_compartilhada) {
	if (shmdt(memoria_compartilhada) == -1) {
		fprintf(stderr, "shmdt failed\n");
        exit(EXIT_FAILURE);
    }
    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        fprintf(stderr, "shmctl(IPC_RMID) failed\n");
        exit(EXIT_FAILURE);
    }
}

void inicializarSemaforoEscritor(struct shared_memo *bufferCompartilhado) {
	// cria semaforo e inicializa com 1 se já nao existir
	if (!bufferCompartilhado->sem_id_writer) {
		bufferCompartilhado->sem_id_writer = semget((key_t)777, 1, 0666 | IPC_CREAT);
		set_semvalue(bufferCompartilhado->sem_id_writer, 0);
	} 
}

void inicializarSemaforoLeitor(struct shared_memo *bufferCompartilhado) {
	// cria semaforo e inicializa com 1 se já nao existir
	if (!bufferCompartilhado->sem_id_reader) {
		bufferCompartilhado->sem_id_reader = semget((key_t)888, 1, 0666 | IPC_CREAT);
		set_semvalue(bufferCompartilhado->sem_id_reader, 0);
		bufferCompartilhado->qtdLeitores = 0;
	} else {
	// se já existe semáforo de leitura, soma mais 1 ao valor do semáforo
		// set_semvalue(bufferCompartilhado->sem_id_reader, 1);
	}
}

/* semaphore_p changes the semaphore by -1 (waiting). */

int semaphore_p(int sem_id)
{
    struct sembuf sem_b;
    
    sem_b.sem_num = 0;
    sem_b.sem_op = -1; /* P() */
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id, &sem_b, 1) == -1) {
        fprintf(stderr, "semaphore_p failed\n");
        return(0);
    }
    return(1);
}

/* semaphore_v is similar except for setting the sem_op part of the sembuf structure to 1,
 so that the semaphore becomes available. */

int semaphore_v(int sem_id)
{
    struct sembuf sem_b;
    
    sem_b.sem_num = 0;
    sem_b.sem_op = 1; /* V() */
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id, &sem_b, 1) == -1) {
        fprintf(stderr, "semaphore_v failed\n");
        return(0);
    }
    return(1);
}

int set_semvalue(int sem_id, int somar) {
    union semun sem_union;
	
	if (!somar)
		sem_union.val = 1;
	else
		sem_union.val = get_semvalue(sem_id) + 1;
    if (semctl(sem_id, 0, SETVAL, sem_union) == -1) return(0);
    return(1);
}

int get_semvalue(int sem_id) {
	union semun sem_union;
	sem_union.array = malloc(4);
	if (semctl(sem_id, 0, GETALL, sem_union) == -1) return (0);
	return sem_union.array[0];
}

void del_semvalue(int sem_id) {
    union semun sem_union;
    
    if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
        fprintf(stderr, "Failed to delete semaphore\n");
}

void terminarPrograma(int shmid, void *memoriaCompartilhada, struct shared_memo* bufferCompartilhado) {
	del_semvalue(bufferCompartilhado->sem_id_writer);
	del_semvalue(bufferCompartilhado->sem_id_reader);
	deletarMemoriaCompartilhada(shmid, memoriaCompartilhada);
}

