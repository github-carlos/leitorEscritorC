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
void deletarMemoriaCompartilhada(int shmid, void *memoria_compartilhada);

void inicializarSemaforoEscritor(struct shared_memo *bufferCompartilhado);
void inicializarSemaforoLeitor(struct shared_memo *bufferCompartilhado);

int set_semvalue(int sem_id);
void del_semvalue(int sem_id);

void terminarPrograma(int shmid, void *memoriaCompartilhada, struct shared_memo* bufferCompartilhado);

int main() {
	
	// guardará o endereço do primeiro byte da memória compartilhada
	void *memoriaCompartilhada = (void *) 0;
	//srand((unsigned int)getpid());  
	
	// serve para guardar o endereço da struct compartilhada
	struct shared_memo *bufferCompartilhado;
	int shmid;
	shmid = criarMemoriaCompartilhada();
	memoriaCompartilhada = associarEspacoMemoAoProcesso(shmid);
    
    printf("Memory attached at %X\n", (int)memoriaCompartilhada);
	
	bufferCompartilhado = (struct shared_memo *) memoriaCompartilhada;
	
	inicializarSemaforoEscritor(bufferCompartilhado);
	inicializarSemaforoLeitor(bufferCompartilhado);
	
	printf("%i", bufferCompartilhado->sem_id_writer);
	bufferCompartilhado->texto[0] = 'o';
	bufferCompartilhado->texto[1] = 'e';
	printf("%s", bufferCompartilhado->texto);
	
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
		set_semvalue(bufferCompartilhado->sem_id_writer);
	}
}

void inicializarSemaforoLeitor(struct shared_memo *bufferCompartilhado) {
	// cria semaforo e inicializa com 1 se já nao existir
	if (!bufferCompartilhado->sem_id_reader) {
		bufferCompartilhado->sem_id_reader = semget((key_t)888, 1, 0666 | IPC_CREAT);
		set_semvalue(bufferCompartilhado->sem_id_reader);
	}
}

int set_semvalue(int sem_id) {
    union semun sem_union;

    sem_union.val = 1;
    if (semctl(sem_id, 0, SETVAL, sem_union) == -1) return(0);
    return(1);
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
