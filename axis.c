#define _POSIX_C_SOURCE 200809L
#include "axis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ═════════════════════════════════════════════════════════════════════════════
//  Tipos internos
// ═════════════════════════════════════════════════════════════════════════════

typedef struct {
    EstadoAxis *e;
    char      **saveptr;
} Ctx;

typedef void (*FnComando)(Ctx *ctx);

typedef struct {
    const char *nome;
    FnComando   fn;
} EntradaComando;

// ═════════════════════════════════════════════════════════════════════════════
//  Utilitários internos
// ═════════════════════════════════════════════════════════════════════════════

static inline char *proximo_token(Ctx *ctx) {
    return strtok_r(NULL, " \t\n\r", ctx->saveptr);
}

static void dbg(EstadoAxis *e, const char *cmd) {
    if (!e->verbose) return;
    printf("[DBG] CMD: %-12s | PTR=%04d | VALOR=%03d | PROFUNDIDADE=%d\n",
           cmd,
           (int)(e->ptr - e->fita),
           (int)*(e->ptr),
           e->call_depth);
    fflush(stdout);
}

// ─── Buscas ──────────────────────────────────────────────────────────────────

static long buscar_rotulo(EstadoAxis *e, const char *nome) {
    for (int i = 0; i < e->total_rotulos; i++) {
        if (strcmp(e->rotulos[i].nome, nome) == 0)
            return e->rotulos[i].posicao;
    }
    fprintf(stderr, "ERRO AXIS: Rotulo '%s' nao encontrado!\n", nome);
    return -1;
}

static int buscar_variavel(EstadoAxis *e, const char *nome) {
    for (int i = e->total_variaveis - 1; i >= 0; i--) {
        if (strcmp(e->variaveis[i].nome, nome) == 0)
            return e->variaveis[i].endereco;
    }
    return -1;
}

static Funcao *buscar_funcao(EstadoAxis *e, const char *nome) {
    for (int i = 0; i < e->total_funcoes; i++) {
        if (strcmp(e->funcoes[i].nome, nome) == 0)
            return &e->funcoes[i];
    }
    return NULL;
}

static void pular_para_rotulo(EstadoAxis *e, const char *nome) {
    long pos = buscar_rotulo(e, nome);
    if (pos != -1 && e->arquivo)
        fseek(e->arquivo, pos, SEEK_SET);
}

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

static int alocar_temp(EstadoAxis *e) {
    if (e->prox_temp < (TAPE_SIZE / 2)) {
        fprintf(stderr, "ERRO AXIS: Memoria de parametros esgotada!\n");
        return -1;
    }
    return e->prox_temp--;
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
    char *arg = strtok_r(NULL, "", ctx->saveptr);
    if (arg) {
        while (*arg == ' ' || *arg == '\t') arg++;
        if (arg[0] == '"') {
            char *fim = strrchr(arg, '"');
            if (fim > arg) {
                for (char *p = arg + 1; p < fim; p++)
                    putchar(*p);
                putchar('\n');
                fflush(stdout);
            }
        } else {
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
        int c; while ((c = getchar()) != '\n' && c != EOF);
    }
    int c; while ((c = getchar()) != '\n' && c != EOF);
}

static void cmd_direita(Ctx *ctx) {
    EstadoAxis *e = ctx->e;
    if (e->ptr < e->fita + TAPE_SIZE - 1) e->ptr++;
    else fprintf(stderr, "AVISO AXIS: Limite direito da fita atingido!\n");
}

static void cmd_esquerda(Ctx *ctx) {
    EstadoAxis *e = ctx->e;
    if (e->ptr > e->fita) e->ptr--;
    else fprintf(stderr, "AVISO AXIS: Limite esquerdo da fita atingido!\n");
}

static void cmd_soma(Ctx *ctx) {
    char *val_str = proximo_token(ctx);
    if (!val_str) return;
    *ctx->e->ptr += resolver_valor(ctx->e, val_str);
}

static void cmd_subtrair(Ctx *ctx) {
    char *val_str = proximo_token(ctx);
    if (!val_str) return;
    *ctx->e->ptr -= resolver_valor(ctx->e, val_str);
}

static void cmd_ir(Ctx *ctx) {
    EstadoAxis *e = ctx->e;
    char *alvo = proximo_token(ctx);
    if (!alvo) return;
    int pos = resolver_endereco(e, alvo);
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

static void cmd_chamar(Ctx *ctx) {
    EstadoAxis *e = ctx->e;

    char *nome_func = proximo_token(ctx);
    if (!nome_func) {
        fprintf(stderr, "ERRO AXIS: CHAMAR exige o nome da funcao\n");
        return;
    }

    Funcao *fn = buscar_funcao(e, nome_func);
    if (!fn) {
        fprintf(stderr, "ERRO AXIS: Funcao '%s' nao encontrada!\n", nome_func);
        return;
    }

    if (e->call_depth >= MAX_CALL_STACK) {
        fprintf(stderr, "ERRO AXIS: Limite de recursao atingido!\n");
        return;
    }

    unsigned char valores[MAX_PARAMS];
    for (int i = 0; i < fn->total_params; i++) {
        char *arg = proximo_token(ctx);
        if (!arg) {
            fprintf(stderr, "ERRO AXIS: Funcao '%s' espera %d argumento(s), faltou argumento %d\n",
                    nome_func, fn->total_params, i + 1);
            return;
        }
        valores[i] = resolver_valor(e, arg);
    }

    e->call_stack[e->call_depth]     = ftell(e->arquivo);
    e->vars_ao_entrar[e->call_depth] = e->total_variaveis;
    e->ptr_ao_entrar[e->call_depth]  = e->ptr;
    e->call_depth++;

    for (int i = 0; i < fn->total_params; i++) {
        int celula = alocar_temp(e);
        if (celula == -1) { e->call_depth--; return; }
        e->fita[celula] = valores[i];
        if (e->total_variaveis < MAX_VARIAVEIS) {
            e->variaveis[e->total_variaveis].endereco = celula;
            strncpy(e->variaveis[e->total_variaveis].nome,
                    fn->params[i], MAX_NOME - 1);
            e->variaveis[e->total_variaveis].nome[MAX_NOME - 1] = '\0';
            e->total_variaveis++;
        }
    }

    if (e->arquivo)
        fseek(e->arquivo, fn->posicao, SEEK_SET);

    if (e->verbose)
        printf("[DBG] CHAMAR '%s' com %d arg(s) | profundidade=%d\n",
               nome_func, fn->total_params, e->call_depth);
}

static void cmd_retornar(Ctx *ctx) {
    EstadoAxis *e = ctx->e;

    if (e->call_depth <= 0) {
        fprintf(stderr, "AVISO AXIS: RETORNAR fora de uma funcao, ignorado.\n");
        return;
    }

    e->call_depth--;
    e->total_variaveis = e->vars_ao_entrar[e->call_depth];
    e->ptr             = e->ptr_ao_entrar[e->call_depth];

    if (e->arquivo)
        fseek(e->arquivo, e->call_stack[e->call_depth], SEEK_SET);

    if (e->verbose)
        printf("[DBG] RETORNAR | profundidade agora=%d\n", e->call_depth);
}

static void cmd_fimfuncao(Ctx *ctx) {
    if (ctx->e->call_depth > 0)
        cmd_retornar(ctx);
}

static void cmd_funcao(Ctx *ctx) {
    char *tok;
    while ((tok = proximo_token(ctx)) != NULL) (void)tok;
}

static void cmd_debug(Ctx *ctx) {
    ctx->e->verbose = 1;
    printf("[DBG] Modo debug ativado.\n");
}

static void cmd_nodebug(Ctx *ctx) {
    ctx->e->verbose = 0;
}

static void cmd_rotulo(Ctx *ctx) {
    proximo_token(ctx);
}

static void cmd_carregar(Ctx *ctx) { (void)ctx; }
static void cmd_repete(Ctx *ctx)   { (void)ctx; }

// ═════════════════════════════════════════════════════════════════════════════
//  Tabela de despacho
// ═════════════════════════════════════════════════════════════════════════════

static const EntradaComando TABELA[] = {
    {"MAIS",       cmd_mais},
    {"MENOS",      cmd_menos},
    {"ZERAR",      cmd_zerar},
    {"SOMA",       cmd_soma},
    {"SOMAR",      cmd_soma},
    {"SUBTRAIR",   cmd_subtrair},
    {"SUBTRAI",    cmd_subtrair},
    {"DIREITA",    cmd_direita},
    {"ESQUERDA",   cmd_esquerda},
    {"IR",         cmd_ir},
    {"COPIAR",     cmd_copiar},
    {"NOMEAR",     cmd_nomear},
    {"PULAR",      cmd_pular},
    {"SE_IGUAL",   cmd_se_igual},
    {"SE_MENOR",   cmd_se_menor},
    {"SE_MAIOR",   cmd_se_maior},
    {"ROTULO",     cmd_rotulo},
    {"ENTRADA",    cmd_entrada},
    {"VALOR",      cmd_valor},
    {"FALAR",      cmd_falar},
    {"FUNCAO",     cmd_funcao},
    {"FIMFUNCAO",  cmd_fimfuncao},
    {"CHAMAR",     cmd_chamar},
    {"RETORNAR",   cmd_retornar},
    {"SAIR",       cmd_sair},
    {"CARREGAR",   cmd_carregar},
    {"REPETE",     cmd_repete},
    {"DEBUG",      cmd_debug},
    {"NODEBUG",    cmd_nodebug},
    {NULL, NULL}
};

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

    rewind(e->arquivo);

    while (fscanf(e->arquivo, "%63s", token) != EOF) {
        if (strcmp(token, "ROTULO") == 0) {
            if (e->total_rotulos >= MAX_ROTULOS) {
                fprintf(stderr, "ERRO AXIS: Limite de rotulos atingido!\n");
                break;
            }
            if (fscanf(e->arquivo, "%63s", token) != 1) break;
            { int c; while ((c = fgetc(e->arquivo)) != '\n' && c != EOF); }
            long posicao = ftell(e->arquivo);
            snprintf(e->rotulos[e->total_rotulos].nome, MAX_NOME, "%s", token);
            e->rotulos[e->total_rotulos].posicao = posicao;
            e->total_rotulos++;
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Mapeamento de funções
// ═════════════════════════════════════════════════════════════════════════════

void axis_mapear_funcoes(EstadoAxis *e) {
    char linha[MAX_LINHA];

    rewind(e->arquivo);

    while (fgets(linha, sizeof(linha), e->arquivo)) {
        char *com = strchr(linha, '$');
        if (com) *com = '\0';

        char *saveptr = NULL;
        char *token = strtok_r(linha, " \t\n\r", &saveptr);
        if (!token) continue;
        if (strcmp(token, "FUNCAO") != 0) continue;

        if (e->total_funcoes >= MAX_FUNCOES) {
            fprintf(stderr, "ERRO AXIS: Limite de funcoes atingido!\n");
            break;
        }

        char *nome = strtok_r(NULL, " \t\n\r", &saveptr);
        if (!nome) continue;

        Funcao *fn = &e->funcoes[e->total_funcoes];
        strncpy(fn->nome, nome, MAX_NOME - 1);
        fn->nome[MAX_NOME - 1] = '\0';
        fn->total_params = 0;

        char *param;
        while ((param = strtok_r(NULL, " \t\n\r", &saveptr)) != NULL) {
            if (fn->total_params >= MAX_PARAMS) {
                fprintf(stderr, "AVISO AXIS: Funcao '%s' excede %d parametros, ignorando extras\n",
                        fn->nome, MAX_PARAMS);
                break;
            }
            strncpy(fn->params[fn->total_params], param, MAX_NOME - 1);
            fn->params[fn->total_params][MAX_NOME - 1] = '\0';
            fn->total_params++;
        }

        fn->posicao = ftell(e->arquivo);
        e->total_funcoes++;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Interpretação de linha
// ═════════════════════════════════════════════════════════════════════════════

void axis_interpretar_linha(EstadoAxis *e, char *linha) {
    // Remove \r para compatibilidade com arquivos Windows (CRLF) em modo binário
    char *cr = strchr(linha, '\r');
    if (cr) *cr = '\n';

    char *comentario = strchr(linha, '$');
    if (comentario) *comentario = '\0';

    int so_espacos = 1;
    for (char *p = linha; *p; p++) {
        if (!isspace((unsigned char)*p)) { so_espacos = 0; break; }
    }
    if (so_espacos) return;

    char linha_original[MAX_LINHA];
    strncpy(linha_original, linha, MAX_LINHA - 1);
    linha_original[MAX_LINHA - 1] = '\0';

    char *saveptr = NULL;
    char *token = strtok_r(linha, " \t\n\r", &saveptr);

    while (token != NULL) {

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
                        if (fn) { dbg(e, sub); fn(&ctx2); }
                        else fprintf(stderr, "AVISO AXIS: Comando desconhecido em REPETE: '%s'\n", sub);
                        sub = strtok_r(NULL, " \t\n\r", &sp2);
                    }
                }
            }
            break;
        }

        if (strcmp(token, "CARREGAR") == 0) {
            char *nome_arquivo = strtok_r(NULL, " \t\n\r", &saveptr);
            if (nome_arquivo) {
                FILE *arq_anterior = e->arquivo;
                FILE *arq = fopen(nome_arquivo, "r");
                if (arq) {
                    e->arquivo = arq;
                    axis_mapear_rotulos(e);
                    axis_mapear_funcoes(e);
                    rewind(arq);
                    char linha_arq[MAX_LINHA];
                    while (fgets(linha_arq, sizeof(linha_arq), arq))
                        axis_interpretar_linha(e, linha_arq);
                    fclose(arq);
                    e->arquivo = arq_anterior;
                } else {
                    fprintf(stderr, "ERRO AXIS: Nao foi possivel abrir '%s'\n", nome_arquivo);
                }
            }
            return;
        }

        Ctx ctx = {e, &saveptr};
        FnComando fn = buscar_comando(token);

        if (fn) {
            dbg(e, token);
            fn(&ctx);
            if (strcmp(token, "FALAR")     == 0 ||
                strcmp(token, "PULAR")     == 0 ||
                strcmp(token, "SE_IGUAL")  == 0 ||
                strcmp(token, "SE_MENOR")  == 0 ||
                strcmp(token, "SE_MAIOR")  == 0 ||
                strcmp(token, "RETORNAR")  == 0 ||
                strcmp(token, "FIMFUNCAO") == 0 ||
                strcmp(token, "FUNCAO")    == 0) {
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
    e->ptr       = e->fita;
    e->prox_temp = TAPE_SIZE - 1;
}

int axis_executar_arquivo(EstadoAxis *e, const char *caminho) {
    e->arquivo = fopen(caminho, "rb");
    if (!e->arquivo) {
        fprintf(stderr, "ERRO AXIS: Nao foi possivel abrir '%s'\n", caminho);
        return -1;
    }

    // Detecta encoding: rejeita UTF-16 (FF FE ou FE FF)
    {
        unsigned char bom2[2] = {0};
        if (fread(bom2, 1, 2, e->arquivo) == 2 &&
            ((bom2[0] == 0xFF && bom2[1] == 0xFE) ||
             (bom2[0] == 0xFE && bom2[1] == 0xFF))) {
            fprintf(stderr, "ERRO AXIS: Arquivo em UTF-16. Salve como UTF-8 sem BOM.\n");
            fclose(e->arquivo);
            e->arquivo = NULL;
            return -1;
        }
        rewind(e->arquivo);
    }

    axis_mapear_rotulos(e);
    axis_mapear_funcoes(e);
    rewind(e->arquivo);

    // Remove BOM UTF-8 (EF BB BF) se presente
    {
        unsigned char bom[3] = {0};
        if (fread(bom, 1, 3, e->arquivo) == 3 &&
            bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) {
            // BOM consumido, continua da posição atual
        } else {
            rewind(e->arquivo); // Não era BOM, volta ao início
        }
    }

    char linha[MAX_LINHA];
    while (fgets(linha, sizeof(linha), e->arquivo)) {
        char copia[MAX_LINHA];
        strncpy(copia, linha, MAX_LINHA - 1);
        copia[MAX_LINHA - 1] = '\0';
        char *com = strchr(copia, '$');
        if (com) *com = '\0';

        char *saveptr = NULL;
        char *primeiro = strtok_r(copia, " \t\n\r", &saveptr);
        if (primeiro && strcmp(primeiro, "FUNCAO") == 0 && e->call_depth == 0) {
            while (fgets(linha, sizeof(linha), e->arquivo)) {
                char *com2 = strchr(linha, '$');
                if (com2) *com2 = '\0';
                char *p = linha;
                while (*p == ' ' || *p == '\t') p++;
                if (strncmp(p, "FIMFUNCAO", 9) == 0) break;
            }
            continue;
        }

        long pos_antes = ftell(e->arquivo);
        axis_interpretar_linha(e, linha);
        // Se axis_interpretar_linha fez um fseek (ex: CHAMAR/RETORNAR),
        // o próximo fgets deve continuar da nova posição — que já está certa.
        // Se NÃO houve salto, a posição continua normal.
        // Não precisamos fazer nada: fgets usa a posição atual do FILE*.
        (void)pos_antes;
    }

    fclose(e->arquivo);
    e->arquivo = NULL;
    return 0;
}