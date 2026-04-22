#define _POSIX_C_SOURCE 200809L
#include "axis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ═════════════════════════════════════════════════════════════════════════════
//  Tipos internos
// ═════════════════════════════════════════════════════════════════════════════

// Contexto passado para cada função de comando
typedef struct {
    EstadoAxis *e;       // Estado completo do interpretador
    char      **saveptr; // Ponteiro de estado do strtok_r atual
} Ctx;

// Assinatura de todos os comandos
typedef void (*FnComando)(Ctx *ctx);

typedef struct {
    const char *nome;
    FnComando   fn;
} EntradaComando;

// ═════════════════════════════════════════════════════════════════════════════
//  Utilitários internos
// ═════════════════════════════════════════════════════════════════════════════

// Lê o próximo token da linha em processamento
static inline char *proximo_token(Ctx *ctx) {
    return strtok_r(NULL, " \t\n\r", ctx->saveptr);
}

// Log de debug (só imprime se verbose estiver ligado)
static void dbg(EstadoAxis *e, const char *cmd) {
    if (!e->verbose) return;
    printf("[DBG] CMD: %-12s | PTR=%04d | VALOR=%03d\n",
           cmd,
           (int)(e->ptr - e->fita),
           (int)*(e->ptr));
    fflush(stdout);
}

// Resolve um token como *endereço* (posição na fita):
// se for nome de variável, retorna o endereço cadastrado;
// caso contrário, interpreta como literal numérico.

// Busca um rótulo e retorna sua posição no arquivo, ou -1 se não encontrado
static long buscar_rotulo(EstadoAxis *e, const char *nome) {
    for (int i = 0; i < e->total_rotulos; i++) {
        if (strcmp(e->rotulos[i].nome, nome) == 0)
            return e->rotulos[i].posicao;
    }
    fprintf(stderr, "ERRO AXIS: Rotulo '%s' nao encontrado!\n", nome);
    return -1;
}

// Busca uma variável e retorna seu endereço na fita, ou -1 se não encontrada
static int buscar_variavel(EstadoAxis *e, const char *nome) {
    for (int i = 0; i < e->total_variaveis; i++) {
        if (strcmp(e->variaveis[i].nome, nome) == 0)
            return e->variaveis[i].endereco;
    }
    return -1;
}

// Pula para um rótulo no arquivo
static void pular_para_rotulo(EstadoAxis *e, const char *nome) {
    long pos = buscar_rotulo(e, nome);
    if (pos != -1 && e->arquivo)
        fseek(e->arquivo, pos, SEEK_SET);
}

// Resolve um token: se for nome de variável, retorna o valor na fita;
// caso contrário, interpreta como literal numérico.
static unsigned char resolver_valor(EstadoAxis *e, const char *token) {
    int endereco = buscar_variavel(e, token);
    if (endereco != -1)
        return e->fita[endereco];
    return (unsigned char)atoi(token);
}


static int resolver_endereco(EstadoAxis *e, const char *token) {
    int endereco = buscar_variavel(e, token);
    if (endereco != -1)
        return endereco;
    return atoi(token);
}


// ═════════════════════════════════════════════════════════════════════════════
//  Implementação de cada comando
// ═════════════════════════════════════════════════════════════════════════════

static void cmd_mais(Ctx *ctx)     { (*ctx->e->ptr)++; }
static void cmd_menos(Ctx *ctx)    { (*ctx->e->ptr)--; }
static void cmd_zerar(Ctx *ctx)    { *ctx->e->ptr = 0; }
static void cmd_sair(Ctx *ctx)     { (void)ctx; exit(0); }

static void cmd_valor(Ctx *ctx) {
    printf("%d\n", (int)*ctx->e->ptr);
    fflush(stdout);
}

static void cmd_falar(Ctx *ctx) {
    // Pega o restante da linha como argumento
    char *arg = strtok_r(NULL, "", ctx->saveptr);
    if (arg) {
        // Pula espaços iniciais
        while (*arg == ' ' || *arg == '\t') arg++;

        if (arg[0] == '"') {
            // Modo string: imprime entre aspas
            char *fim = strrchr(arg, '"');
            if (fim > arg) {
                for (char *p = arg + 1; p < fim; p++)
                    putchar(*p);
                putchar('\n');
                fflush(stdout);
            }
        } else {
            // Modo caractere: imprime o valor da célula atual
            printf("%c", *ctx->e->ptr);
            fflush(stdout);
        }
    } else {
        printf("%c", *ctx->e->ptr);
        fflush(stdout);
    }
}

static void cmd_entrada(Ctx *ctx) {
    int temp;
    printf("Digite um numero (0-255): ");
    fflush(stdout);

    if (scanf("%d", &temp) == 1) {
        *ctx->e->ptr = (unsigned char)(temp & 0xFF);
    } else {
        fprintf(stderr, "ERRO AXIS: Entrada invalida.\n");
        // Limpa o buffer de entrada
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }
    // Descarta o '\n' restante (apenas um getchar é suficiente)
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void cmd_direita(Ctx *ctx) {
    EstadoAxis *e = ctx->e;
    if (e->ptr < e->fita + TAPE_SIZE - 1)
        e->ptr++;
    else
        fprintf(stderr, "AVISO AXIS: Limite direito da fita atingido!\n");
}

static void cmd_esquerda(Ctx *ctx) {
    EstadoAxis *e = ctx->e;
    if (e->ptr > e->fita)
        e->ptr--;
    else
        fprintf(stderr, "AVISO AXIS: Limite esquerdo da fita atingido!\n");
}

static void cmd_soma(Ctx *ctx) {
    char *val_str = proximo_token(ctx);
    if (!val_str) return;

    // 1. Tenta ver se é uma variável
    int endereco = buscar_variavel(ctx->e, val_str);
    unsigned char valor_para_somar;

    if (endereco != -1) {
        // Se achou a variável, pega o valor que está na fita naquele endereço
        valor_para_somar = ctx->e->fita[endereco];
    } else {
        // Se não achou, trata como número direto (literal)
        valor_para_somar = (unsigned char)atoi(val_str);
    }

    *ctx->e->ptr += valor_para_somar;
}

static void cmd_subtrair(Ctx *ctx) {
    char *val_str = proximo_token(ctx);
    if (!val_str) return;

    int endereco = buscar_variavel(ctx->e, val_str);
    unsigned char valor_para_sub;

    if (endereco != -1) {
        valor_para_sub = ctx->e->fita[endereco];
    } else {
        valor_para_sub = (unsigned char)atoi(val_str);
    }

    *ctx->e->ptr -= valor_para_sub;
}

static void cmd_ir(Ctx *ctx) {
    EstadoAxis *e = ctx->e;
    char *alvo = proximo_token(ctx);
    if (!alvo) return;

    // Tenta resolver como nome de variável primeiro
    int pos = buscar_variavel(e, alvo);

    // Se não encontrou, tenta interpretar como número direto
    if (pos == -1) pos = atoi(alvo);

    if (pos >= 0 && pos < TAPE_SIZE)
        e->ptr = &e->fita[pos];
    else
        fprintf(stderr, "ERRO AXIS: Posicao invalida em IR: %d\n", pos);
}

static void cmd_nomear(Ctx *ctx) {
    EstadoAxis *e = ctx->e;
    char *pos_str  = proximo_token(ctx);
    char *nome_str = proximo_token(ctx);

    if (!pos_str || !nome_str) {
        fprintf(stderr, "ERRO AXIS: NOMEAR exige <posicao> <nome>\n");
        return;
    }
    if (e->total_variaveis >= MAX_VARIAVEIS) {
        fprintf(stderr, "ERRO AXIS: Limite de variaveis atingido!\n");
        return;
    }

    e->variaveis[e->total_variaveis].endereco = atoi(pos_str);
    strncpy(e->variaveis[e->total_variaveis].nome, nome_str, MAX_NOME - 1);
    e->variaveis[e->total_variaveis].nome[MAX_NOME - 1] = '\0';
    e->total_variaveis++;
}

static void cmd_pular(Ctx *ctx) {
    char *alvo = proximo_token(ctx);
    if (alvo) pular_para_rotulo(ctx->e, alvo);
}

static void cmd_se_igual(Ctx *ctx) {
    char *val_str = proximo_token(ctx);
    char *alvo    = proximo_token(ctx);
    if (!val_str || !alvo) return;

    if (*ctx->e->ptr == resolver_valor(ctx->e, val_str))
        pular_para_rotulo(ctx->e, alvo);
}

static void cmd_se_menor(Ctx *ctx) {
    char *val_str = proximo_token(ctx);
    char *alvo    = proximo_token(ctx);
    if (!val_str || !alvo) return;

    if (*ctx->e->ptr < resolver_valor(ctx->e, val_str))
        pular_para_rotulo(ctx->e, alvo);
}

static void cmd_se_maior(Ctx *ctx) {
    char *val_str = proximo_token(ctx);
    char *alvo    = proximo_token(ctx);
    if (!val_str || !alvo) return;

    if (*ctx->e->ptr > resolver_valor(ctx->e, val_str))
        pular_para_rotulo(ctx->e, alvo);
}

static void cmd_copiar(Ctx *ctx) {
    EstadoAxis *e = ctx->e;
    char *orig_str = proximo_token(ctx);
    char *dest_str = proximo_token(ctx);
    if (!orig_str || !dest_str) {
        fprintf(stderr, "ERRO AXIS: COPIAR exige <origem> <destino>\n");
        return;
    }

    int orig = resolver_endereco(e, orig_str);
    int dest = resolver_endereco(e, dest_str);

    if (orig < 0 || orig >= TAPE_SIZE || dest < 0 || dest >= TAPE_SIZE) {
        fprintf(stderr, "ERRO AXIS: Posicao fora dos limites em COPIAR\n");
        return;
    }
    e->fita[dest] = e->fita[orig];
}

// NOVO: Ativa o modo debug para as próximas instruções
static void cmd_debug(Ctx *ctx) {
    ctx->e->verbose = 1;
    printf("[DBG] Modo debug ativado.\n");
}

// NOVO: Desativa o modo debug
static void cmd_nodebug(Ctx *ctx) {
    ctx->e->verbose = 0;
}

// ─── CARREGAR é tratado na interpretar_linha, mas precisa de entrada na tabela
//     para não gerar "comando desconhecido". A função fica vazia aqui pois
//     o despacho especial acontece antes de chamar a tabela.
static void cmd_rotulo(Ctx *ctx) {
    // Consome o nome do rótulo para não ser interpretado como comando
    proximo_token(ctx);
}

static void cmd_carregar(Ctx *ctx) {
    // Tratamento real está em axis_interpretar_linha
    (void)ctx;
}

static void cmd_repete(Ctx *ctx) {
    // Tratamento real está em axis_interpretar_linha
    (void)ctx;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Tabela de despacho
// ═════════════════════════════════════════════════════════════════════════════

static const EntradaComando TABELA[] = {
    // Aritmética
    {"MAIS",      cmd_mais},
    {"MENOS",     cmd_menos},
    {"ZERAR",     cmd_zerar},
    {"SOMA",      cmd_soma},
    {"SOMAR",     cmd_soma},      // Alias
    {"SUBTRAIR",  cmd_subtrair},
    {"SUBTRAI",   cmd_subtrair},  // Alias

    // Movimentação na fita
    {"DIREITA",   cmd_direita},
    {"ESQUERDA",  cmd_esquerda},
    {"IR",        cmd_ir},
    {"COPIAR",    cmd_copiar},    // NOVO

    // Variáveis
    {"NOMEAR",    cmd_nomear},

    // Fluxo de controle
    {"PULAR",     cmd_pular},
    {"SE_IGUAL",  cmd_se_igual},
    {"SE_MENOR",  cmd_se_menor}, // NOVO
    {"SE_MAIOR",  cmd_se_maior}, // NOVO
    {"ROTULO",    cmd_rotulo},

    // I/O
    {"ENTRADA",   cmd_entrada},
    {"VALOR",     cmd_valor},
    {"FALAR",     cmd_falar},

    // Sistema
    {"SAIR",      cmd_sair},
    {"CARREGAR",  cmd_carregar},
    {"REPETE",    cmd_repete},
    {"DEBUG",     cmd_debug},    // NOVO
    {"NODEBUG",   cmd_nodebug},  // NOVO

    {NULL, NULL} // Sentinela
};

// Busca um comando na tabela e retorna o ponteiro de função, ou NULL
static FnComando buscar_comando(const char *nome) {
    for (int i = 0; TABELA[i].nome != NULL; i++) {
        if (strcmp(TABELA[i].nome, nome) == 0)
            return TABELA[i].fn;
    }
    return NULL;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Mapeamento de rótulos
// ═════════════════════════════════════════════════════════════════════════════

void axis_mapear_rotulos(EstadoAxis *e) {
    char token[MAX_NOME];
    long posicao;

    rewind(e->arquivo);

    while (fscanf(e->arquivo, "%63s", token) != EOF) {
        if (strcmp(token, "ROTULO") == 0) {
            if (e->total_rotulos >= MAX_ROTULOS) {
                fprintf(stderr, "ERRO AXIS: Limite de rotulos atingido!\n");
                break;
            }
            if (fscanf(e->arquivo, "%63s", token) != 1) break;
            // Avança até o fim da linha do ROTULO para que o salto
            // aterrise na linha SEGUINTE, não no nome do rótulo
            { int c; while ((c = fgetc(e->arquivo)) != '\n' && c != EOF); }
            posicao = ftell(e->arquivo);

            snprintf(e->rotulos[e->total_rotulos].nome, MAX_NOME, "%s", token);
            e->rotulos[e->total_rotulos].posicao = posicao;
            e->total_rotulos++;
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Interpretação de linha
// ═════════════════════════════════════════════════════════════════════════════

void axis_interpretar_linha(EstadoAxis *e, char *linha) {
    // 1. Remove comentários (tudo após '$')
    char *comentario = strchr(linha, '$');
    if (comentario) *comentario = '\0';

    // 2. Ignora linhas vazias
    int so_espacos = 1;
    for (char *p = linha; *p; p++) {
        if (!isspace((unsigned char)*p)) { so_espacos = 0; break; }
    }
    if (so_espacos) return;

    // 3. Guarda cópia para o REPETE (que precisa da linha original)
    char linha_original[MAX_LINHA];
    strncpy(linha_original, linha, MAX_LINHA - 1);
    linha_original[MAX_LINHA - 1] = '\0';

    char *saveptr = NULL;
    char *token = strtok_r(linha, " \t\n\r", &saveptr);

    while (token != NULL) {

        // ── Tratamento especial: REPETE ───────────────────────────────────
        if (strcmp(token, "REPETE") == 0) {
            char *vezes_str = strtok_r(NULL, " \t\n\r", &saveptr);
            int vezes = vezes_str ? atoi(vezes_str) : 0;

            char *inicio = strchr(linha_original, '[');
            char *fim    = strrchr(linha_original, ']');

            if (inicio && fim && fim > inicio) {
                *fim = '\0';
                char *bloco = inicio + 1;

                for (int i = 0; i < vezes; i++) {
                    char copia[MAX_LINHA];
                    strncpy(copia, bloco, MAX_LINHA - 1);
                    copia[MAX_LINHA - 1] = '\0';

                    char *sp2 = NULL;
                    char *sub = strtok_r(copia, " \t\n\r", &sp2);
                    while (sub) {
                        Ctx ctx2 = {e, &sp2};
                        FnComando fn = buscar_comando(sub);
                        if (fn) {
                            dbg(e, sub);
                            fn(&ctx2);
                        } else {
                            fprintf(stderr, "AVISO AXIS: Comando desconhecido em REPETE: '%s'\n", sub);
                        }
                        sub = strtok_r(NULL, " \t\n\r", &sp2);
                    }
                }
            }
            // Avança para depois do ']' na linha original e para de processar
            break;
        }

        // ── Tratamento especial: CARREGAR ─────────────────────────────────
        if (strcmp(token, "CARREGAR") == 0) {
            char *nome_arquivo = strtok_r(NULL, " \t\n\r", &saveptr);
            if (nome_arquivo) {
                FILE *arq = fopen(nome_arquivo, "r");
                if (arq) {
                    char linha_arq[MAX_LINHA];
                    while (fgets(linha_arq, sizeof(linha_arq), arq))
                        axis_interpretar_linha(e, linha_arq);
                    fclose(arq);
                } else {
                    fprintf(stderr, "ERRO AXIS: Nao foi possivel abrir '%s'\n", nome_arquivo);
                }
            }
            return;
        }

        // ── Despacho normal pela tabela ───────────────────────────────────
        Ctx ctx = {e, &saveptr};
        FnComando fn = buscar_comando(token);

        if (fn) {
            dbg(e, token);
            fn(&ctx);

            // Após FALAR, PULAR ou condicionais, a linha termina
            if (strcmp(token, "FALAR")    == 0 ||
                strcmp(token, "PULAR")    == 0 ||
                strcmp(token, "SE_IGUAL") == 0 ||
                strcmp(token, "SE_MENOR") == 0 ||
                strcmp(token, "SE_MAIOR") == 0) {
                return;
            }
        } else {
            fprintf(stderr, "AVISO AXIS: Comando desconhecido: '%s'\n", token);
        }

        token = strtok_r(NULL, " \t\n\r", &saveptr);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Inicialização e execução
// ═════════════════════════════════════════════════════════════════════════════

void axis_init(EstadoAxis *e) {
    memset(e, 0, sizeof(*e));
    e->ptr = e->fita;
}

int axis_executar_arquivo(EstadoAxis *e, const char *caminho) {
    e->arquivo = fopen(caminho, "r");
    if (!e->arquivo) {
        fprintf(stderr, "ERRO AXIS: Nao foi possivel abrir '%s'\n", caminho);
        return -1;
    }

    axis_mapear_rotulos(e);
    rewind(e->arquivo);

    char linha[MAX_LINHA];
    while (fgets(linha, sizeof(linha), e->arquivo))
        axis_interpretar_linha(e, linha);

    fclose(e->arquivo);
    e->arquivo = NULL;
    return 0;
}