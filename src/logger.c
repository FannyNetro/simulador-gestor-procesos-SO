#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/logger.h"

// ─── Crea el logger ───────────────────────────────────────────────────────────
Logger* logger_crear() {
    Logger* log = (Logger*)malloc(sizeof(Logger));
    if (!log) return NULL;

    log->primero  = NULL;
    log->ultimo   = NULL;
    log->cantidad = 0;
    log->inicio   = time(NULL);

    return log;
}

// ─── Registra un evento ───────────────────────────────────────────────────────
void logger_registrar(Logger* log, int pid, const char* nombre,
                      TipoEvento tipo, const char* descripcion) {
    if (!log) return;

    NodoEvento* evento = (NodoEvento*)malloc(sizeof(NodoEvento));
    if (!evento) return;

    evento->pid       = pid;
    evento->tipo      = tipo;
    evento->timestamp = time(NULL);
    evento->siguiente = NULL;

    strncpy(evento->nombre,      nombre,      63);
    strncpy(evento->descripcion, descripcion, 127);
    evento->nombre[63]      = '\0';
    evento->descripcion[127] = '\0';

    // Agregar al final de la lista
    if (!log->ultimo) {
        log->primero = evento;
        log->ultimo  = evento;
    } else {
        log->ultimo->siguiente = evento;
        log->ultimo            = evento;
    }
    log->cantidad++;
}

// ─── Imprime el historial de eventos ─────────────────────────────────────────
void logger_imprimir(const Logger* log) {
    if (!log || !log->primero) {
        printf("[LOG] No hay eventos registrados\n");
        return;
    }

    printf("\n========================================================\n");
    printf("  REGISTRO DE EVENTOS (%d en total)\n", log->cantidad);
    printf("========================================================\n");
    printf(" %-8s | %-6s | %-15s | %s\n",
           "TIEMPO", "PID", "PROCESO", "EVENTO");
    printf("--------------------------------------------------------\n");

    NodoEvento* actual = log->primero;
    while (actual) {
        // Calcular tiempo relativo al inicio
        double segundos = difftime(actual->timestamp, log->inicio);

        printf(" %07.3fs | %-6d | %-15s | %s\n",
               segundos,
               actual->pid,
               actual->nombre,
               actual->descripcion);

        actual = actual->siguiente;
    }
    printf("========================================================\n\n");
}

// ─── Destruye el logger y libera memoria ─────────────────────────────────────
void logger_destruir(Logger* log) {
    if (!log) return;

    NodoEvento* actual = log->primero;
    while (actual) {
        NodoEvento* siguiente = actual->siguiente;
        free(actual);
        actual = siguiente;
    }
    free(log);
}