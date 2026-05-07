#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/process.h"

// ─── Contador global de PIDs ──────────────────────────────────────────────────
static int siguiente_pid = 1;

// ─── Crea un nuevo proceso ────────────────────────────────────────────────────
PCB* proceso_crear(const char* nombre, int prioridad, int rafaga_cpu, int memoria_mb) {
    PCB* pcb = (PCB*)malloc(sizeof(PCB));
    if (!pcb) return NULL;

    pcb->pid        = siguiente_pid++;
    pcb->prioridad  = prioridad;
    pcb->rafaga_cpu = rafaga_cpu;
    pcb->memoria_mb = memoria_mb;
    pcb->estado     = NUEVO;
    pcb->creado_en  = time(NULL);
    strncpy(pcb->nombre, nombre, 63);
    pcb->nombre[63] = '\0';

    return pcb;
}

// ─── Valida y aplica una transición de estado ─────────────────────────────────
int proceso_transicion(PCB* pcb, EstadoProceso nuevo_estado) {
    if (!pcb) return -1;

    // Transiciones válidas
    int valida = 0;
    switch (pcb->estado) {
        case NUEVO:       valida = (nuevo_estado == LISTO);      break;
        case LISTO:       valida = (nuevo_estado == EJECUTANDO); break;
        case EJECUTANDO:  valida = (nuevo_estado == LISTO     ||
                                   nuevo_estado == ESPERANDO  ||
                                   nuevo_estado == TERMINADO); break;
        case ESPERANDO:   valida = (nuevo_estado == LISTO);      break;
        case TERMINADO:   valida = 0;                            break;
    }

    if (!valida) {
        printf("[ERROR] Transicion ilegal: %s -> %s\n",
               estado_a_texto(pcb->estado),
               estado_a_texto(nuevo_estado));
        return -1;
    }

    pcb->estado = nuevo_estado;
    return 0;
}

// ─── Termina un proceso con una causa ────────────────────────────────────────
void proceso_terminar(PCB* pcb, CausaTerminacion causa) {
    if (!pcb) return;
    pcb->causa_fin = causa;
    proceso_transicion(pcb, TERMINADO);
}

// ─── Convierte estado a texto ─────────────────────────────────────────────────
const char* estado_a_texto(EstadoProceso estado) {
    switch (estado) {
        case NUEVO:      return "NUEVO";
        case LISTO:      return "LISTO";
        case EJECUTANDO: return "EJECUTANDO";
        case ESPERANDO:  return "ESPERANDO";
        case TERMINADO:  return "TERMINADO";
        default:         return "DESCONOCIDO";
    }
}

// ─── Imprime la información del PCB ──────────────────────────────────────────
void proceso_imprimir(const PCB* pcb) {
    if (!pcb) return;
    printf("PID=%-3d | %-15s | %-10s | Prioridad=%-2d | Rafaga=%-4dms | Mem=%-4dMB\n",
           pcb->pid,
           pcb->nombre,
           estado_a_texto(pcb->estado),
           pcb->prioridad,
           pcb->rafaga_cpu,
           pcb->memoria_mb);
}