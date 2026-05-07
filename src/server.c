#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/server.h"
#include "../include/process.h"
#include "../include/resource.h"
#include "../include/scheduler.h"
#include "../include/logger.h"

// ─── Referencia al tipo Simulador ─────────────────────────────────────────────
typedef struct {
    RecursoPool* recursos;
    Scheduler*   scheduler;
    Logger*      logger;
    PCB*         procesos[100];
    int          total_procesos;
} Simulador;

// ─── Genera JSON con estado del simulador ─────────────────────────────────────
static void generar_json_estado(Simulador* sim, char* buf, int tam) {
    int offset = 0;
    offset += snprintf(buf + offset, tam - offset,
        "{\"algoritmo\":\"%s\","
        "\"cpu_libre\":%d,"
        "\"memoria_libre\":%d,",
        algoritmo_a_texto(sim->scheduler->algoritmo),
        sim->recursos->cpu_cores_libres,
        sim->recursos->memoria_libre_mb);

    // Procesos
    offset += snprintf(buf + offset, tam - offset, "\"procesos\":[");
    for (int i = 0; i < sim->total_procesos; i++) {
        PCB* p = sim->procesos[i];
        if (!p) continue;
        if (i > 0) offset += snprintf(buf + offset, tam - offset, ",");
        offset += snprintf(buf + offset, tam - offset,
            "{\"pid\":%d,\"nombre\":\"%s\","
            "\"estado\":\"%s\",\"prioridad\":%d,"
            "\"rafaga\":%d,\"memoria\":%d}",
            p->pid, p->nombre,
            estado_a_texto(p->estado),
            p->prioridad, p->rafaga_cpu, p->memoria_mb);
    }
    offset += snprintf(buf + offset, tam - offset, "],");

    // Log de eventos
    offset += snprintf(buf + offset, tam - offset, "\"eventos\":[");
    NodoEvento* ev = sim->logger->primero;
    int primero = 1;
    while (ev) {
        if (!primero)
            offset += snprintf(buf + offset, tam - offset, ",");
        offset += snprintf(buf + offset, tam - offset,
            "{\"pid\":%d,\"nombre\":\"%s\",\"desc\":\"%s\"}",
            ev->pid, ev->nombre, ev->descripcion);
        primero = 0;
        ev = ev->siguiente;
    }
    offset += snprintf(buf + offset, tam - offset, "]}");
}

// ─── Manejador de peticiones HTTP ─────────────────────────────────────────────
static void manejador(struct mg_connection* c, int ev, void* ev_data) {
    Simulador* sim = (Simulador*)c->fn_data;

    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message* hm = (struct mg_http_message*)ev_data;

        // ── GET /api/estado ───────────────────────────────────────────────────
        if (mg_match(hm->uri, mg_str("/api/estado"), NULL)) {
            char json[8192];
            generar_json_estado(sim, json, sizeof(json));
            mg_http_reply(c, 200,
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n",
                "%s", json);

        // ── POST /api/crear ───────────────────────────────────────────────────
        } else if (mg_match(hm->uri, mg_str("/api/crear"), NULL)) {
            char nombre[64], prioridad_s[8], rafaga_s[8], memoria_s[8];
            mg_http_get_var(&hm->body, "nombre",    nombre,      sizeof(nombre));
            mg_http_get_var(&hm->body, "prioridad", prioridad_s, sizeof(prioridad_s));
            mg_http_get_var(&hm->body, "rafaga",    rafaga_s,    sizeof(rafaga_s));
            mg_http_get_var(&hm->body, "memoria",   memoria_s,   sizeof(memoria_s));

            int prioridad = atoi(prioridad_s);
            int rafaga    = atoi(rafaga_s);
            int memoria   = atoi(memoria_s);

            if (recursos_solicitar(sim->recursos, 1, memoria) == 0) {
                PCB* pcb = proceso_crear(nombre, prioridad, rafaga, memoria);
                sim->procesos[sim->total_procesos++] = pcb;
                scheduler_encolar(sim->scheduler, pcb);

                char desc[128];
                snprintf(desc, sizeof(desc),
                         "Creado -> NUEVO -> LISTO | mem=%dMB rafaga=%dms",
                         memoria, rafaga);
                logger_registrar(sim->logger, pcb->pid, pcb->nombre,
                                 EVT_CREADO, desc);

                mg_http_reply(c, 200,
                    "Content-Type: application/json\r\n"
                    "Access-Control-Allow-Origin: *\r\n",
                    "{\"ok\":true,\"pid\":%d}", pcb->pid);
            } else {
                mg_http_reply(c, 200,
                    "Content-Type: application/json\r\n"
                    "Access-Control-Allow-Origin: *\r\n",
                    "{\"ok\":false,\"error\":\"Recursos insuficientes\"}");
            }

        // ── POST /api/terminar ────────────────────────────────────────────────
        } else if (mg_match(hm->uri, mg_str("/api/terminar"), NULL)) {
            char pid_s[8];
            mg_http_get_var(&hm->body, "pid", pid_s, sizeof(pid_s));
            int pid = atoi(pid_s);

            for (int i = 0; i < sim->total_procesos; i++) {
                PCB* p = sim->procesos[i];
                if (p && p->pid == pid && p->estado != TERMINADO) {
                    recursos_liberar(sim->recursos, 1, p->memoria_mb);
                    proceso_terminar(p, CAUSA_USUARIO);
                    char desc[128];
                    snprintf(desc, sizeof(desc),
                             "Terminado por usuario | mem=%dMB liberados",
                             p->memoria_mb);
                    logger_registrar(sim->logger, p->pid, p->nombre,
                                     EVT_TERMINADO, desc);
                    mg_http_reply(c, 200,
                        "Content-Type: application/json\r\n"
                        "Access-Control-Allow-Origin: *\r\n",
                        "{\"ok\":true}");
                    return;
                }
            }
            mg_http_reply(c, 200,
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n",
                "{\"ok\":false,\"error\":\"Proceso no encontrado\"}");

        // ── POST /api/despachar ───────────────────────────────────────────────
        } else if (mg_match(hm->uri, mg_str("/api/despachar"), NULL)) {
            PCB* pcb = scheduler_siguiente(sim->scheduler);
            if (pcb) {
                proceso_transicion(pcb, EJECUTANDO);
                char desc[128];
                snprintf(desc, sizeof(desc), "Despachado -> EJECUTANDO (%s)",
                         algoritmo_a_texto(sim->scheduler->algoritmo));
                logger_registrar(sim->logger, pcb->pid, pcb->nombre,
                                 EVT_TRANSICION, desc);
                mg_http_reply(c, 200,
                    "Content-Type: application/json\r\n"
                    "Access-Control-Allow-Origin: *\r\n",
                    "{\"ok\":true,\"pid\":%d}", pcb->pid);
            } else {
                mg_http_reply(c, 200,
                    "Content-Type: application/json\r\n"
                    "Access-Control-Allow-Origin: *\r\n",
                    "{\"ok\":false,\"error\":\"Cola vacia\"}");
            }

        // ── POST /api/algoritmo ───────────────────────────────────────────────
        } else if (mg_match(hm->uri, mg_str("/api/algoritmo"), NULL)) {
            char algo_s[8], quantum_s[8];
            mg_http_get_var(&hm->body, "algoritmo", algo_s,    sizeof(algo_s));
            mg_http_get_var(&hm->body, "quantum",   quantum_s, sizeof(quantum_s));

            int algo    = atoi(algo_s);
            int quantum = atoi(quantum_s);
            if (quantum <= 0) quantum = 4;

            Algoritmo algoritmos[] = {FCFS, SJF, ROUND_ROBIN, PRIORIDAD};
            if (algo >= 0 && algo <= 3) {
                sim->scheduler->algoritmo = algoritmos[algo];
                sim->scheduler->quantum   = quantum;
                char desc[128];
                snprintf(desc, sizeof(desc), "Algoritmo cambiado a %s",
                         algoritmo_a_texto(sim->scheduler->algoritmo));
                logger_registrar(sim->logger, -1, "SISTEMA",
                                 EVT_ALGORITMO_CAMBIADO, desc);
            }
            mg_http_reply(c, 200,
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n",
                "{\"ok\":true}");

        // ── Archivos estáticos (interfaz web) ─────────────────────────────────
        } else {
            struct mg_http_serve_opts opts = {.root_dir = "web"};
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}

// ─── Inicia el servidor HTTP ──────────────────────────────────────────────────
void servidor_iniciar(void* simulador) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0:3000", manejador, simulador);
    printf("[SERVIDOR] Interfaz disponible en http://localhost:8080\n");
    for (;;) mg_mgr_poll(&mgr, 100);
    mg_mgr_free(&mgr);
}