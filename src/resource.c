#include <stdio.h>
#include <stdlib.h>
#include "../include/resource.h"

// ─── Crea el pool de recursos ─────────────────────────────────────────────────
RecursoPool* recursos_crear() {
    RecursoPool* pool = (RecursoPool*)malloc(sizeof(RecursoPool));
    if (!pool) return NULL;

    pool->cpu_cores_libres  = 1;      // 1 núcleo de CPU
    pool->memoria_libre_mb  = 4096;   // 4 GB de memoria

    return pool;
}

// ─── Solicita recursos ────────────────────────────────────────────────────────
int recursos_solicitar(RecursoPool* pool, int cpu, int memoria_mb) {
    if (!pool) return -1;

    if (pool->cpu_cores_libres < cpu) {
        printf("[RECURSOS] CPU no disponible (%d libre)\n", 
               pool->cpu_cores_libres);
        return -1;
    }

    if (pool->memoria_libre_mb < memoria_mb) {
        printf("[RECURSOS] Memoria insuficiente (%d MB libres)\n", 
               pool->memoria_libre_mb);
        return -1;
    }

    pool->cpu_cores_libres  -= cpu;
    pool->memoria_libre_mb  -= memoria_mb;
    return 0;
}

// ─── Libera recursos ──────────────────────────────────────────────────────────
void recursos_liberar(RecursoPool* pool, int cpu, int memoria_mb) {
    if (!pool) return;

    pool->cpu_cores_libres += cpu;
    if (pool->cpu_cores_libres > 1)
        pool->cpu_cores_libres = 1;

    pool->memoria_libre_mb += memoria_mb;
    if (pool->memoria_libre_mb > 4096)
        pool->memoria_libre_mb = 4096;
}

// ─── Imprime estado de recursos ───────────────────────────────────────────────
void recursos_imprimir(const RecursoPool* pool) {
    if (!pool) return;
    printf("[RECURSOS] CPU: %d libre | Memoria: %d MB / 4096 MB\n",
           pool->cpu_cores_libres,
           pool->memoria_libre_mb);
}