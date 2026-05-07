#ifndef PROCESS_H
#define PROCESS_H

#include <time.h>

// ─── Estados posibles de un proceso ───────────────────────────────────────────
typedef enum {
    NUEVO,
    LISTO,
    EJECUTANDO,
    ESPERANDO,
    TERMINADO
} EstadoProceso;

// ─── Causas de terminación ────────────────────────────────────────────────────
typedef enum {
    NORMAL,
    ERROR,
    INTERBLOQUEO,
    USUARIO
} CausaTerminacion;

// ─── Bloque de Control de Proceso (PCB) ───────────────────────────────────────
typedef struct {
    int             pid;            // Identificador único del proceso
    char            nombre[64];     // Nombre del proceso
    EstadoProceso   estado;         // Estado actual
    int             prioridad;      // 0 = mayor prioridad
    int             rafaga_cpu;     // Ráfaga estimada en ms
    int             memoria_mb;     // Memoria solicitada en MB
    CausaTerminacion causa_fin;     // Causa de terminación
    time_t          creado_en;      // Timestamp de creación
} PCB;

// ─── Funciones del módulo ─────────────────────────────────────────────────────
PCB*  proceso_crear(const char* nombre, int prioridad, int rafaga_cpu, int memoria_mb);
int   proceso_transicion(PCB* pcb, EstadoProceso nuevo_estado);
void  proceso_terminar(PCB* pcb, CausaTerminacion causa);
void  proceso_imprimir(const PCB* pcb);
const char* estado_a_texto(EstadoProceso estado);

#endif // PROCESS_H