#ifndef AXIS_H
#define AXIS_H

#include <stdio.h>

// ─── Constantes ───────────────────────────────────────────────────────────────
#define TAPE_SIZE       30000
#define MAX_ROTULOS     512
#define MAX_VARIAVEIS   512
#define MAX_NOME        64
#define MAX_LINHA       512

// ─── Estruturas ───────────────────────────────────────────────────────────────
typedef struct {
    char nome[MAX_NOME];
    long posicao;
} Rotulo;

typedef struct {
    char nome[MAX_NOME];
    int  endereco;
} Variavel;

// ─── Estado global do interpretador ──────────────────────────────────────────
typedef struct {
    unsigned char  fita[TAPE_SIZE];
    unsigned char *ptr;             // Ponteiro atual na fita

    Rotulo    rotulos[MAX_ROTULOS];
    int       total_rotulos;

    Variavel  variaveis[MAX_VARIAVEIS];
    int       total_variaveis;

    FILE     *arquivo;
    int       verbose;              // Modo debug (-v)
} EstadoAxis;

// ─── API pública ──────────────────────────────────────────────────────────────
void axis_init(EstadoAxis *e);
void axis_mapear_rotulos(EstadoAxis *e);
int  axis_executar_arquivo(EstadoAxis *e, const char *caminho);
void axis_interpretar_linha(EstadoAxis *e, char *linha);

#endif // AXIS_H
