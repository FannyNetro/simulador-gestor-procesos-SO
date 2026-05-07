#ifndef IPC_H
#define IPC_H

#include <pthread.h>

// ─── Semáforo contador ────────────────────────────────────────────────────────
typedef struct {
    int             valor;      // Valor actual del semáforo
    pthread_mutex_t mutex;      // Mutex para proteger el valor
    pthread_cond_t  condicion;  // Para bloquear/despertar hilos
} Semaforo;

// ─── Buffer para productor-consumidor ────────────────────────────────────────
#define BUFFER_TAMANO 3

typedef struct {
    int      items[BUFFER_TAMANO];  // Contenido del buffer
    int      cantidad;              // Cuántos items hay ahorita
    int      entrada;               // Índice donde se produce
    int      salida;                // Índice donde se consume
    Semaforo mutex;                 // Exclusión mutua
    Semaforo vacios;                // Espacios vacíos disponibles
    Semaforo llenos;                // Espacios llenos disponibles
} Buffer;

// ─── Funciones del semáforo ───────────────────────────────────────────────────
int  semaforo_crear(Semaforo* sem, int valor_inicial);
void semaforo_wait(Semaforo* sem);    // P() — decrementa, bloquea si es 0
void semaforo_signal(Semaforo* sem);  // V() — incrementa, despierta un hilo
void semaforo_destruir(Semaforo* sem);

// ─── Funciones del buffer ─────────────────────────────────────────────────────
Buffer* buffer_crear();
void    buffer_producir(Buffer* b, int item, int pid_productor);
int     buffer_consumir(Buffer* b, int pid_consumidor);
void    buffer_destruir(Buffer* b);

#endif // IPC_H