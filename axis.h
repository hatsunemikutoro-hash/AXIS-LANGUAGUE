#ifndef AXIS_H
#define AXIS_H

#include <stdio.h>

// --- Suas Definições de Dados ---
typedef struct {
    char nome[20];
    int endereco;
} Variavel;

typedef struct {
    char nome[50];
    long posicao;
} Rotulo;

// --- Variáveis Globais (Avisando que elas existem) ---
extern FILE *arquivo_global;
extern Rotulo mapa_de_rotulos[100];
extern int total_rotulos;

// --- Assinaturas das Funções ---
void mapear_rotulo(FILE *arquivo);
long buscar_linha_do_rotulo(char *nome);
void executar(char *comando, unsigned char **ptr_ref, unsigned char *tapebase, Variavel *agenda, int *total);
void interpretar_linha(char *linha, unsigned char **ptr_ref, unsigned char *tapebase, Variavel *agenda, int *total);

#endif