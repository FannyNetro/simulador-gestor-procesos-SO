#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

// ─── Algoritmos de planificación disponibles ──────────────────────────────────
typedef enum {
    FCFS,        // First Come First Served
    SJF,         // Shortest Job First
    ROUND_ROBIN, // Round Robin con quantum configurable
    PRIORIDAD    // Por prioridad (0 = mayor prioridad)
} Algoritmo;

// ─── Cola de listos ───────────────────────────────────────────────────────────
typedef struct NodoCola {
    PCB*             pcb;
    struct NodoCola* siguiente;
} NodoCola;

typedef struct {
    NodoCola* frente;
    NodoCola* final;
    int       cantidad;
    Algoritmo algoritmo;
    int       quantum;      // Solo usado en Round Robin (ms)
} Scheduler;

// ─── Funciones del módulo ─────────────────────────────────────────────────────
Scheduler* scheduler_crear(Algoritmo algoritmo, int quantum);
void       scheduler_encolar(Scheduler* s, PCB* pcb);
PCB*       scheduler_siguiente(Scheduler* s);
void       scheduler_imprimir_cola(const Scheduler* s);
const char* algoritmo_a_texto(Algoritmo algoritmo);

#endif // SCHEDULER_H