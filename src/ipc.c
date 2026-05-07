#include <stdio.h>
#include <stdlib.h>
#include "../include/ipc.h"

// ─── Crea e inicializa un semáforo ────────────────────────────────────────────
int semaforo_crear(Semaforo* sem, int valor_inicial) {
    if (!sem) return -1;

    sem->valor = valor_inicial;
    pthread_mutex_init(&sem->mutex, NULL);
    pthread_cond_init(&sem->condicion, NULL);
    return 0;
}

// ─── Wait / P() — decrementa, bloquea si valor es 0 ──────────────────────────
void semaforo_wait(Semaforo* sem) {
    pthread_mutex_lock(&sem->mutex);
    while (sem->valor == 0)
        pthread_cond_wait(&sem->condicion, &sem->mutex);
    sem->valor--;
    pthread_mutex_unlock(&sem->mutex);
}

// ─── Signal / V() — incrementa y despierta un hilo en espera ─────────────────
void semaforo_signal(Semaforo* sem) {
    pthread_mutex_lock(&sem->mutex);
    sem->valor++;
    pthread_cond_signal(&sem->condicion);
    pthread_mutex_unlock(&sem->mutex);
}

// ─── Destruye el semáforo ─────────────────────────────────────────────────────
void semaforo_destruir(Semaforo* sem) {
    if (!sem) return;
    pthread_mutex_destroy(&sem->mutex);
    pthread_cond_destroy(&sem->condicion);
}

// ─── Crea el buffer productor-consumidor ─────────────────────────────────────
Buffer* buffer_crear() {
    Buffer* b = (Buffer*)malloc(sizeof(Buffer));
    if (!b) return NULL;

    b->cantidad = 0;
    b->entrada  = 0;
    b->salida   = 0;

    semaforo_crear(&b->mutex,  1);               // Exclusión mutua
    semaforo_crear(&b->vacios, BUFFER_TAMANO);   // Espacios vacíos = capacidad
    semaforo_crear(&b->llenos, 0);               // Espacios llenos = 0

    return b;
}

// ─── Produce un item en el buffer ─────────────────────────────────────────────
void buffer_producir(Buffer* b, int item, int pid_productor) {
    semaforo_wait(&b->vacios);   // Espera si buffer lleno
    semaforo_wait(&b->mutex);    // Entra a sección crítica

    b->items[b->entrada] = item;
    b->entrada = (b->entrada + 1) % BUFFER_TAMANO;
    b->cantidad++;

    printf("[PID=%d | Productor] produce -> item_%d (buffer: %d/%d)\n",
           pid_productor, item, b->cantidad, BUFFER_TAMANO);

    semaforo_signal(&b->mutex);  // Sale de sección crítica
    semaforo_signal(&b->llenos); // Avisa que hay un item más
}

// ─── Consume un item del buffer ───────────────────────────────────────────────
int buffer_consumir(Buffer* b, int pid_consumidor) {
    semaforo_wait(&b->llenos);   // Espera si buffer vacío
    semaforo_wait(&b->mutex);    // Entra a sección crítica

    int item = b->items[b->salida];
    b->salida = (b->salida + 1) % BUFFER_TAMANO;
    b->cantidad--;

    printf("[PID=%d | Consumidor] consume <- item_%d (buffer: %d/%d)\n",
           pid_consumidor, item, b->cantidad, BUFFER_TAMANO);

    semaforo_signal(&b->mutex);  // Sale de sección crítica
    semaforo_signal(&b->vacios); // Avisa que hay un espacio más

    return item;
}

// ─── Destruye el buffer ───────────────────────────────────────────────────────
void buffer_destruir(Buffer* b) {
    if (!b) return;
    semaforo_destruir(&b->mutex);
    semaforo_destruir(&b->vacios);
    semaforo_destruir(&b->llenos);
    free(b);
}
