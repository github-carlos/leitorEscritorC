#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// biblioteca de memoria compartilhada
#include <sys/shm.h>

#include "estruturaCompartilhada.h"

int criarMemoriaCompartilhada();
void *associarEspacoMemoAoProcesso(int shmid);

void inicializarSemaforoEscritor(struct shared_memo *bufferCompartilhado);
void inicializarSemaforoLeitor(struct shared_memo *bufferCompartilhado);

int set_semvalue(int sem_id);
void del_semvalue(int sem_id);

void terminarPrograma(int shmid, void *memoriaCompartilhada, struct shared_memo* bufferCompartilhado);

int main() {
		
		void *memoria_compartilhada = (void *)0;
		struct shared_memo *bufferCompartilhado;
		int shmid;
		
		shmid = criarMemoriaCompartilhada();
		memoria_compartilhada = associarEspacoMemoAoProcesso(shmid);
		
		printf("Memory attached at %X\n", (int)memoria_compartilhada);
		
		bufferCompartilhado = (struct shared_memo *) memoria_compartilhada;
		
		printf("%s\n", bufferCompartilhado->texto);
			if (bufferCompartilhado->sem_id_reader == NULL) {
		printf("é nulo %i\n", bufferCompartilhado->sem_id_reader);
	} else {
		printf("não é nulo %i\n", bufferCompartilhado->sem_id_writer);
		printf("não é nulo %i\n", bufferCompartilhado->sem_id_reader);
	}
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

