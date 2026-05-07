#ifndef SERVER_H
#define SERVER_H

#include "mongoose.h"
#include "process.h"
#include "resource.h"
#include "scheduler.h"
#include "logger.h"

// ─── Estructura que comparte el simulador con el servidor ─────────────────────
typedef struct {
    void*  simulador;   // Puntero al simulador principal
    int    corriendo;   // 1 = servidor activo
} ServidorCtx;

// ─── Funciones del módulo ─────────────────────────────────────────────────────
void servidor_iniciar(void* simulador);

#endif // SERVER_H