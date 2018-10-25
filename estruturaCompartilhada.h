/* Arquivo para descrever estrutura que será compartilhada no buffer*/

#define TEXT_SZ 2048

struct shared_memo {
	 // guarda id do semáforo de escrita
	 int sem_id_writer;
	 // guarda id do semáforo de leitura
	 int sem_id_reader;
	 
	 int qtdLeitores;
	char texto[TEXT_SZ];
};
