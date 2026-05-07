#include <stdio.h>
#include <stdlib.h>
#include "../include/scheduler.h"

// ─── Crea el scheduler ────────────────────────────────────────────────────────
Scheduler* scheduler_crear(Algoritmo algoritmo, int quantum) {
    Scheduler* s = (Scheduler*)malloc(sizeof(Scheduler));
    if (!s) return NULL;

    s->frente    = NULL;
    s->final     = NULL;
    s->cantidad  = 0;
    s->algoritmo = algoritmo;
    s->quantum   = quantum;

    return s;
}

// ─── Encola un proceso ────────────────────────────────────────────────────────
void scheduler_encolar(Scheduler* s, PCB* pcb) {
    if (!s || !pcb) return;

    NodoCola* nodo = (NodoCola*)malloc(sizeof(NodoCola));
    if (!nodo) return;

    nodo->pcb       = pcb;
    nodo->siguiente = NULL;

    proceso_transicion(pcb, LISTO);

    if (!s->final) {
        s->frente = nodo;
        s->final  = nodo;
    } else {
        s->final->siguiente = nodo;
        s->final            = nodo;
    }
    s->cantidad++;
}

// ─── Devuelve el siguiente proceso según el algoritmo ────────────────────────
PCB* scheduler_siguiente(Scheduler* s) {
    if (!s || !s->frente) return NULL;

    NodoCola* elegido_nodo     = s->frente;
    NodoCola* anterior_elegido = NULL;

    // SJF y PRIORIDAD buscan el mejor candidato en toda la cola
    if (s->algoritmo == SJF || s->algoritmo == PRIORIDAD) {
        NodoCola* actual    = s->frente->siguiente;
        NodoCola* anterior  = s->frente;

        while (actual) {
            int mejor = 0;
            if (s->algoritmo == SJF)
                mejor = actual->pcb->rafaga_cpu < elegido_nodo->pcb->rafaga_cpu;
            else
                mejor = actual->pcb->prioridad < elegido_nodo->pcb->prioridad;

            if (mejor) {
                elegido_nodo     = actual;
                anterior_elegido = anterior;
            }
            anterior = actual;
            actual   = actual->siguiente;
        }
    }

    // Extraer el nodo elegido de la cola
    PCB* pcb = elegido_nodo->pcb;

    if (anterior_elegido)
        anterior_elegido->siguiente = elegido_nodo->siguiente;
    else
        s->frente = elegido_nodo->siguiente;

    if (elegido_nodo == s->final)
        s->final = anterior_elegido;

    free(elegido_nodo);
    s->cantidad--;

    return pcb;
}

// ─── Imprime la cola de listos ────────────────────────────────────────────────
void scheduler_imprimir_cola(const Scheduler* s) {
    if (!s || !s->frente) {
        printf("[SCHEDULER] Cola vacia\n");
        return;
    }

    printf("[SCHEDULER] Algoritmo: %s | Cola (%d procesos): ",
           algoritmo_a_texto(s->algoritmo), s->cantidad);

    NodoCola* actual = s->frente;
    while (actual) {
        printf("[PID=%d %s]", actual->pcb->pid, actual->pcb->nombre);
        if (actual->siguiente) printf(" -> ");
        actual = actual->siguiente;
    }
    printf("\n");
}

// ─── Convierte algoritmo a texto ─────────────────────────────────────────────
const char* algoritmo_a_texto(Algoritmo algoritmo) {
    switch (algoritmo) {
        case FCFS:        return "FCFS";
        case SJF:         return "SJF";
        case ROUND_ROBIN: return "Round Robin";
        case PRIORIDAD:   return "Prioridad";
        default:          return "Desconocido";
    }
}