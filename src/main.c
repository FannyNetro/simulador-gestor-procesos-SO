#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../include/process.h"
#include "../include/resource.h"
#include "../include/scheduler.h"
#include "../include/ipc.h"
#include "../include/logger.h"

// ─── Estructura principal del simulador ──────────────────────────────────────
typedef struct {
    RecursoPool* recursos;
    Scheduler*   scheduler;
    Logger*      logger;
    PCB*         procesos[100];
    int          total_procesos;
} Simulador;

// ─── Argumentos para hilos productor/consumidor ───────────────────────────────
typedef struct {
    Buffer* buffer;
    int     pid;
    int     items_a_procesar;
} ArgHilo;

// ─── Prototipos ───────────────────────────────────────────────────────────────
Simulador* simulador_crear(Algoritmo algoritmo, int quantum);
void       simulador_destruir(Simulador* sim);
void       crear_proceso(Simulador* sim);
void       listar_procesos(Simulador* sim);
void       terminar_proceso(Simulador* sim);
void       ver_recursos(Simulador* sim);
void       ver_log(Simulador* sim);
void       cambiar_algoritmo(Simulador* sim);
void       demo_productor_consumidor(Simulador* sim);
void       despachar_siguiente(Simulador* sim);
void       imprimir_menu(Simulador* sim);
void*      hilo_productor(void* arg);
void*      hilo_consumidor(void* arg);

// ─── Limpia el buffer de entrada ─────────────────────────────────────────────
void limpiar_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// ─── Crea el simulador ────────────────────────────────────────────────────────
Simulador* simulador_crear(Algoritmo algoritmo, int quantum) {
    Simulador* sim = (Simulador*)malloc(sizeof(Simulador));
    if (!sim) return NULL;

    sim->recursos       = recursos_crear();
    sim->scheduler      = scheduler_crear(algoritmo, quantum);
    sim->logger         = logger_crear();
    sim->total_procesos = 0;

    logger_registrar(sim->logger, -1, "SISTEMA", EVT_SISTEMA,
                     "Simulador iniciado");
    return sim;
}

// ─── Destruye el simulador ────────────────────────────────────────────────────
void simulador_destruir(Simulador* sim) {
    if (!sim) return;
    for (int i = 0; i < sim->total_procesos; i++)
        if (sim->procesos[i]) free(sim->procesos[i]);
    free(sim->recursos);
    free(sim->scheduler);
    logger_destruir(sim->logger);
    free(sim);
}

// ─── Crea un proceso nuevo ────────────────────────────────────────────────────
void crear_proceso(Simulador* sim) {
    char nombre[64];
    int  prioridad, rafaga, memoria;

    printf("\nNombre del proceso: ");
    fgets(nombre, sizeof(nombre), stdin);
    nombre[strcspn(nombre, "\n")] = '\0';

    printf("Prioridad (0-9, 0=mayor): ");
    scanf("%d", &prioridad);
    printf("Rafaga CPU (ms): ");
    scanf("%d", &rafaga);
    printf("Memoria (MB): ");
    scanf("%d", &memoria);
    limpiar_buffer();

    if (recursos_solicitar(sim->recursos, 1, memoria) != 0) {
        printf("[ERROR] No hay recursos suficientes\n");
        return;
    }

    PCB* pcb = proceso_crear(nombre, prioridad, rafaga, memoria);
    if (!pcb) return;

    sim->procesos[sim->total_procesos++] = pcb;
    scheduler_encolar(sim->scheduler, pcb);

    char desc[128];
    snprintf(desc, sizeof(desc),
             "Creado -> NUEVO -> LISTO | mem=%dMB rafaga=%dms",
             memoria, rafaga);
    logger_registrar(sim->logger, pcb->pid, pcb->nombre, EVT_CREADO, desc);

    printf("[OK] Proceso creado - PID=%d estado=%s mem=%dMB\n",
           pcb->pid, estado_a_texto(pcb->estado), memoria);
}

// ─── Lista todos los procesos ─────────────────────────────────────────────────
void listar_procesos(Simulador* sim) {
    if (sim->total_procesos == 0) {
        printf("\n[INFO] No hay procesos creados\n");
        return;
    }

    printf("\n%-5s | %-15s | %-10s | %-9s | %-8s | %-6s\n",
           "PID", "NOMBRE", "ESTADO", "PRIORIDAD", "RAFAGA", "MEM");
    printf("--------------------------------------------------------------\n");

    for (int i = 0; i < sim->total_procesos; i++) {
        PCB* p = sim->procesos[i];
        if (p) proceso_imprimir(p);
    }
    printf("\n");
    recursos_imprimir(sim->recursos);
    scheduler_imprimir_cola(sim->scheduler);
}

// ─── Termina un proceso por PID ───────────────────────────────────────────────
void terminar_proceso(Simulador* sim) {
    int pid;
    printf("\nPID del proceso a terminar: ");
    scanf("%d", &pid);
    limpiar_buffer();

    for (int i = 0; i < sim->total_procesos; i++) {
        PCB* p = sim->procesos[i];
        if (p && p->pid == pid && p->estado != TERMINADO) {
            recursos_liberar(sim->recursos, 1, p->memoria_mb);
            proceso_terminar(p, USUARIO);

            char desc[128];
            snprintf(desc, sizeof(desc),
                     "Terminado por usuario | mem=%dMB liberados",
                     p->memoria_mb);
            logger_registrar(sim->logger, p->pid, p->nombre,
                             EVT_TERMINADO, desc);

            printf("[OK] Proceso PID=%d terminado\n", pid);
            return;
        }
    }
    printf("[ERROR] Proceso PID=%d no encontrado o ya terminado\n", pid);
}

// ─── Muestra recursos disponibles ────────────────────────────────────────────
void ver_recursos(Simulador* sim) {
    printf("\n");
    recursos_imprimir(sim->recursos);
}

// ─── Muestra el log de eventos ────────────────────────────────────────────────
void ver_log(Simulador* sim) {
    logger_imprimir(sim->logger);
}

// ─── Cambia el algoritmo de planificación ────────────────────────────────────
void cambiar_algoritmo(Simulador* sim) {
    int opcion, quantum = 4;

    printf("\n[1] FCFS  [2] SJF  [3] Round Robin  [4] Prioridad\n");
    printf("Seleccione: ");
    scanf("%d", &opcion);
    limpiar_buffer();

    if (opcion == 3) {
        printf("Quantum (ms): ");
        scanf("%d", &quantum);
        limpiar_buffer();
    }

    Algoritmo algoritmos[] = {FCFS, SJF, ROUND_ROBIN, PRIORIDAD};
    if (opcion >= 1 && opcion <= 4) {
        sim->scheduler->algoritmo = algoritmos[opcion - 1];
        sim->scheduler->quantum   = quantum;

        char desc[128];
        snprintf(desc, sizeof(desc), "Algoritmo cambiado a %s",
                 algoritmo_a_texto(sim->scheduler->algoritmo));
        logger_registrar(sim->logger, -1, "SISTEMA",
                         EVT_ALGORITMO_CAMBIADO, desc);

        printf("[OK] %s\n", desc);
    }
}

// ─── Despacha el siguiente proceso ───────────────────────────────────────────
void despachar_siguiente(Simulador* sim) {
    PCB* pcb = scheduler_siguiente(sim->scheduler);
    if (!pcb) {
        printf("[INFO] Cola vacia, no hay procesos para despachar\n");
        return;
    }

    proceso_transicion(pcb, EJECUTANDO);

    char desc[128];
    snprintf(desc, sizeof(desc), "Despachado -> EJECUTANDO (%s)",
             algoritmo_a_texto(sim->scheduler->algoritmo));
    logger_registrar(sim->logger, pcb->pid, pcb->nombre,
                     EVT_TRANSICION, desc);

    printf("[DISPATCH] PID=%d %s -> EJECUTANDO\n", pcb->pid, pcb->nombre);
}

// ─── Hilo productor ───────────────────────────────────────────────────────────
void* hilo_productor(void* arg) {
    ArgHilo* a = (ArgHilo*)arg;
    for (int i = 0; i < a->items_a_procesar; i++)
        buffer_producir(a->buffer, i, a->pid);
    return NULL;
}

// ─── Hilo consumidor ──────────────────────────────────────────────────────────
void* hilo_consumidor(void* arg) {
    ArgHilo* a = (ArgHilo*)arg;
    for (int i = 0; i < a->items_a_procesar * 2; i++)
        buffer_consumir(a->buffer, a->pid);
    return NULL;
}

// ─── Demo productor-consumidor ────────────────────────────────────────────────
void demo_productor_consumidor(Simulador* sim) {
    printf("\n[IPC] Iniciando demo productor-consumidor\n");
    printf("[IPC] Buffer capacidad: %d | Productores: 2 | Consumidor: 1\n\n",
           BUFFER_TAMANO);

    Buffer* buffer = buffer_crear();

    ArgHilo arg_p1 = { buffer, 10, 5 };
    ArgHilo arg_p2 = { buffer, 11, 5 };
    ArgHilo arg_c1 = { buffer, 12, 5 };

    pthread_t p1, p2, c1;
    pthread_create(&p1, NULL, hilo_productor, &arg_p1);
    pthread_create(&p2, NULL, hilo_productor, &arg_p2);
    pthread_create(&c1, NULL, hilo_consumidor, &arg_c1);

    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
    pthread_join(c1, NULL);

    printf("\n[IPC] Demo finalizada\n");
    logger_registrar(sim->logger, -1, "SISTEMA", EVT_SISTEMA,
                     "Demo productor-consumidor completada");

    buffer_destruir(buffer);
}

// ─── Menu principal ───────────────────────────────────────────────────────────
void imprimir_menu(Simulador* sim) {
    printf("\n=== Simulador de Gestor de Procesos ===\n");
    printf(" [1] Crear proceso\n");
    printf(" [2] Listar procesos\n");
    printf(" [3] Terminar proceso\n");
    printf(" [4] Ver recursos\n");
    printf(" [5] Ver log de eventos\n");
    printf(" [6] Cambiar algoritmo\n");
    printf(" [7] Despachar siguiente proceso\n");
    printf(" [8] Demo productor-consumidor\n");
    printf(" [0] Salir\n");
    printf("Algoritmo activo: %s\n",
           algoritmo_a_texto(sim->scheduler->algoritmo));
    printf("> ");
}

// ─── Punto de entrada ─────────────────────────────────────────────────────────
int main() {
    Simulador* sim = simulador_crear(ROUND_ROBIN, 4);
    if (!sim) {
        printf("[ERROR] No se pudo iniciar el simulador\n");
        return 1;
    }

    int opcion;

    printf("=== Simulador de Gestor de Procesos ===\n");
    printf("Algoritmo inicial: Round Robin (quantum=4ms)\n");

    do {
        imprimir_menu(sim);
        scanf("%d", &opcion);
        limpiar_buffer();

        switch (opcion) {
            case 1: crear_proceso(sim);              break;
            case 2: listar_procesos(sim);            break;
            case 3: terminar_proceso(sim);           break;
            case 4: ver_recursos(sim);               break;
            case 5: ver_log(sim);                    break;
            case 6: cambiar_algoritmo(sim);          break;
            case 7: despachar_siguiente(sim);        break;
            case 8: demo_productor_consumidor(sim);  break;
            case 0: printf("Saliendo...\n");         break;
            default: printf("Opcion no valida\n");   break;
        }
    } while (opcion != 0);

    simulador_destruir(sim);
    return 0;
}