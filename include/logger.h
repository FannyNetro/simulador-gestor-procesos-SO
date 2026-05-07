#ifndef LOGGER_H
#define LOGGER_H

#include <time.h>
#include "process.h"

// ─── Tipos de eventos ─────────────────────────────────────────────────────────
typedef enum {
    EVT_CREADO,
    EVT_TRANSICION,
    EVT_TERMINADO,
    EVT_RECURSO_ASIGNADO,
    EVT_RECURSO_LIBERADO,
    EVT_ALGORITMO_CAMBIADO,
    EVT_SISTEMA
} TipoEvento;

// ─── Estructura de un evento ──────────────────────────────────────────────────
typedef struct NodoEvento {
    int              pid;           // PID del proceso (-1 si es del sistema)
    char             nombre[64];    // Nombre del proceso o "SISTEMA"
    TipoEvento       tipo;          // Tipo de evento
    char             descripcion[128]; // Descripción del evento
    time_t           timestamp;     // Momento en que ocurrió
    struct NodoEvento* siguiente;   // Siguiente evento en la lista
} NodoEvento;

// ─── Logger ───────────────────────────────────────────────────────────────────
typedef struct {
    NodoEvento* primero;    // Primer evento registrado
    NodoEvento* ultimo;     // Último evento registrado
    int         cantidad;   // Total de eventos
    time_t      inicio;     // Momento en que arrancó el simulador
} Logger;

// ─── Funciones del módulo ─────────────────────────────────────────────────────
Logger* logger_crear();
void    logger_registrar(Logger* log, int pid, const char* nombre,
                         TipoEvento tipo, const char* descripcion);
void    logger_imprimir(const Logger* log);
void    logger_destruir(Logger* log);

#endif // LOGGER_H