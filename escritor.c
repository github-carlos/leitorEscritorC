#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// biblioteca de memoria compartilhada
#include <sys/shm.h>
#include <semaphore.h>
#include "semun.h"
#include "estruturaCompartilhada.h"

// métodos para memória compartilhada
int criarMemoriaCompartilhada();
void *associarEspacoMemoAoProcesso(int shmid);
void deletarMemoriaCompartilhada(int shmid, void *memoria_compartilhada);

// métodos para os semáforos
void inicializarSemaforoEscritor(struct shared_memo *bufferCompartilhado);
void inicializarSemaforoLeitor(struct shared_memo *bufferCompartilhado);
int semaphore_p(int sem_id);
int semaphore_v(int sem_id);
int set_semvalue(int sem_id);
int get_semvalue(int sem_id);
void del_semvalue(int sem_id);

// utilidades
void escreverNaMemoria(struct shared_memo* bufferCompartilhado);
char* gerarNumeroAleatorio();
void terminarPrograma(int shmid, void *memoriaCompartilhada, struct shared_memo* bufferCompartilhado);

// variavel auxiliar para escrever na memoria
char str[100];

int main() {
	// guardará o endereço do primeiro byte da memória compartilhada
	void *memoriaCompartilhada = (void *) 0;
	
	// serve para guardar o endereço da struct compartilhada
	struct shared_memo *bufferCompartilhado;
	int shmid;
	shmid = criarMemoriaCompartilhada();
	memoriaCompartilhada = associarEspacoMemoAoProcesso(shmid);
	
    printf("Memory attached at %X\n", (int)memoriaCompartilhada);
    
	bufferCompartilhado = (struct shared_memo *) memoriaCompartilhada;
	
	inicializarSemaforoEscritor(bufferCompartilhado);
	inicializarSemaforoLeitor(bufferCompartilhado);
	
	int running = 1;
	
	while(running) {
		printf("Dormindo...\n");
		sleep(rand() % 10);
		
		// vê se está bloqueado
		if (get_semvalue(bufferCompartilhado->sem_id_writer) == 0) {
			printf("Esperando...\n");
		};
		
		// bloqueia area de dados
		if(!semaphore_p(bufferCompartilhado->sem_id_writer)) exit(EXIT_FAILURE);
		
		// escreve na memoria
		escreverNaMemoria(bufferCompartilhado);
		
		// libera area de dados
		if (!semaphore_v(bufferCompartilhado->sem_id_writer)) exit(EXIT_FAILURE);
		
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

/* Faz o espaço ser acessível ao processo e retorna endereço para o primeiro byte
 * shmid é o id da memoria compartilhada
 * */
void *associarEspacoMemoAoProcesso(int shmid) {
	void *enderecoMemo = shmat(shmid, (void *)0, 0);
	
	if (enderecoMemo == (void *)-1) {
      fprintf(stderr, "shmat failed\n");
      exit(EXIT_FAILURE);
    }
    return enderecoMemo;
}

/* Desassocia memoria compartilhada do processo e deleta
 * shmid é o id da memoria compartilhada
 * bufferCompartilhado é o buffer contendo os valores compartilhados
 * */
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

/* Cria um semáforo para a area de dados caso já nao exista. Inicializa valor com 1
 * bufferCompartilhado é o buffer contendo os valores compartilhados
 * */
void inicializarSemaforoEscritor(struct shared_memo *bufferCompartilhado) {
	if (!bufferCompartilhado->sem_id_writer) {
		bufferCompartilhado->sem_id_writer = semget((key_t)777, 1, 0666 | IPC_CREAT);
		set_semvalue(bufferCompartilhado->sem_id_writer);
	}
}

/* Cria o semáforo leitor caso já não exista e seta o valor para 1
 * e a quantidade de leitores para 0
 * bufferCompartilhado é o buffer contendo os valores compartilhados
 * */
void inicializarSemaforoLeitor(struct shared_memo *bufferCompartilhado) {
	// cria semaforo e inicializa com 1 se já nao existir
	if (!bufferCompartilhado->sem_id_reader) {
		bufferCompartilhado->sem_id_reader = semget((key_t)888, 1, 0666 | IPC_CREAT);
		set_semvalue(bufferCompartilhado->sem_id_reader);
		bufferCompartilhado->qtdLeitores = 0;
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

/* Alterar valor do semáforo
 * sem_id é o id do semáforo
 * retorna 0 se der erro. 1 se não der erro
 * */
int set_semvalue(int sem_id) {
    union semun sem_union;

    sem_union.val = 1;
    if (semctl(sem_id, 0, SETVAL, sem_union) == -1) return(0);
    return(1);
}

/* Pegar valor atual do semáforo.
 * sem_id é o id do semáforo
 * retorna valor
 * */
int get_semvalue(int sem_id) {
	union semun sem_union;
	sem_union.array = malloc(4);
	if (semctl(sem_id, 0, GETALL, sem_union) == -1) return (0);
	return sem_union.array[0];
}

/* Deletar semáforo
 * sem_id id do semáforo a ser deletado
 * */
void del_semvalue(int sem_id) {
    union semun sem_union;
    
    if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
        fprintf(stderr, "Failed to delete semaphore\n");
}

/* Escreve na memória compartilhada o número gerado aleatoriamente
 * bufferCompartilhado é o endereço do primeiro byte da memoria compartilhada
 * */
void escreverNaMemoria(struct shared_memo* bufferCompartilhado) {
	strcpy(bufferCompartilhado->texto, gerarNumeroAleatorio());
	printf("Escreveu: %s\n", bufferCompartilhado->texto);		
}

/* Gera um número aleatório do tipo char[]
 * retorna o endereço da variavel global str
 * */
char* gerarNumeroAleatorio() {
	
	int number = rand() % 1000;
	sprintf(str, "%i", number);
	return str;
}

/* Finaliza o programa deletando os semáforos e liberando a memória compartilhada
 * shmid id da memória compartilhada
 * memoriaCompartilhada é o endereço da memoriaCompartilhada
 * bufferCompartilhado é o buffer com os valores compartilhados
 * */
void terminarPrograma(int shmid, void *memoriaCompartilhada, struct shared_memo* bufferCompartilhado) {
	del_semvalue(bufferCompartilhado->sem_id_writer);
	del_semvalue(bufferCompartilhado->sem_id_reader);
	deletarMemoriaCompartilhada(shmid, memoriaCompartilhada);
}
