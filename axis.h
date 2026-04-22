#ifndef AXIS_H
#define AXIS_H

#include <stdio.h>

// ─── Constantes ───────────────────────────────────────────────────────────────
#define TAPE_SIZE       30000
#define MAX_ROTULOS     512
#define MAX_VARIAVEIS   512
#define MAX_FUNCOES     128
#define MAX_PARAMS      16
#define MAX_CALL_STACK  64
#define MAX_NOME        64
#define MAX_LINHA       512

// ─── Estruturas ───────────────────────────────────────────────────────────────
typedef struct {
    char nome[MAX_NOME];
    long posicao;           // offset no arquivo logo após a linha FUNCAO
} Rotulo;

typedef struct {
    char nome[MAX_NOME];
    int  endereco;
} Variavel;

typedef struct {
    char nome[MAX_NOME];
    char params[MAX_PARAMS][MAX_NOME]; // nomes dos parâmetros
    int  total_params;
    long posicao;           // offset no arquivo logo após a linha FUNCAO
} Funcao;

// ─── Estado global do interpretador ──────────────────────────────────────────
typedef struct {
    unsigned char  fita[TAPE_SIZE];
    unsigned char *ptr;             // Ponteiro atual na fita

    Rotulo    rotulos[MAX_ROTULOS];
    int       total_rotulos;

    Variavel  variaveis[MAX_VARIAVEIS];
    int       total_variaveis;

    Funcao    funcoes[MAX_FUNCOES];
    int       total_funcoes;

    // Pilha de retorno: guarda a posição no arquivo para cada CHAMAR
    long      call_stack[MAX_CALL_STACK];
    // Guarda quantas variáveis existiam ao entrar em cada nível,
    // para poder desfazê-las ao retornar
    int       vars_ao_entrar[MAX_CALL_STACK];
    // Guarda a posição do ptr da fita ao entrar (para restaurar se necessário)
    unsigned char *ptr_ao_entrar[MAX_CALL_STACK];
    int       call_depth;           // profundidade atual da pilha

    // Próxima célula livre para alocação temporária de parâmetros
    // Começa no final da fita e cresce para trás, longe do espaço do usuário
    int       prox_temp;

    FILE     *arquivo;
    int       verbose;              // Modo debug (-v)
} EstadoAxis;

// ─── API pública ──────────────────────────────────────────────────────────────
void axis_init(EstadoAxis *e);
void axis_mapear_rotulos(EstadoAxis *e);
void axis_mapear_funcoes(EstadoAxis *e);
int  axis_executar_arquivo(EstadoAxis *e, const char *caminho);
void axis_interpretar_linha(EstadoAxis *e, char *linha);

#endif // AXIS_H