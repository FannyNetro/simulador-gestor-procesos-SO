#ifndef RESOURCE_H
#define RESOURCE_H

// ─── Pool de recursos del sistema ─────────────────────────────────────────────
typedef struct {
    int cpu_cores_libres;   // Núcleos de CPU disponibles (máx 1)
    int memoria_libre_mb;   // Memoria RAM disponible (máx 4096 MB)
} RecursoPool;

// ─── Funciones del módulo ─────────────────────────────────────────────────────
RecursoPool* recursos_crear();
int          recursos_solicitar(RecursoPool* pool, int cpu, int memoria_mb);
void         recursos_liberar(RecursoPool* pool, int cpu, int memoria_mb);
void         recursos_imprimir(const RecursoPool* pool);

#endif // RESOURCE_H