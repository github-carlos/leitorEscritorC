
bufferCompartilhado = {
    'semaforoEscritor': 0,
    'semaforoLeitor': 0,
    'mensagem': 'oi'
}

# semaforo 0 é indisponível

# quando for escrever semaforo de escrita e leitura devem ser 0
# quando for ler semaforo de escrita deve ser 0

# incrementa para liberar semaphoro
semaphore_v(sem_id)
# decrementa
semaphore_p(sem_id)