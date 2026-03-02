#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct
{
    char nome[20];
    int endereco;
} Variavel;

typedef struct {
    char nome[50];
    long posicao;
} Rotulo;

FILE *arquivo_global = NULL; // A GAMBIARRA DOS DEUSES
Rotulo mapa_de_rotulos[100]; 
int total_rotulos = 0;

void mapear_rotulo(FILE *arquivo) {
    char comando[50];
    char nome_rotulo[50];
    long posicao;

    rewind(arquivo);

    while (fscanf(arquivo, "%s", comando) != EOF)
    {
        if (strcmp(comando, "ROTULO") == 0) {
            posicao = ftell(arquivo);
            fscanf(arquivo, "%s", nome_rotulo);

            strcpy(mapa_de_rotulos[total_rotulos].nome, nome_rotulo);
            mapa_de_rotulos[total_rotulos].posicao = posicao;

            printf("LOG: Rotulo '%s' mapeado na posicao %ld\n", nome_rotulo, posicao);
            total_rotulos++;
        }
    }
    
}

long buscar_linha_do_rotulo(char *nome) {
    for (int i = 0; i < total_rotulos; i++)
    {
        if (strcmp(mapa_de_rotulos[i].nome, nome) == 0) {
            return mapa_de_rotulos[i].posicao;
        }
    }
    printf("ERRO DA AXIS: Rotulo '%s' nao encontrado!\n", nome);
    return -1;
}

void executar(char *comando, unsigned char **ptr_ref, unsigned char *tapebase, Variavel *agenda, int *total) {
    // REGRAS PARA NÃO ERRAR MAIS:
    // (*ptr_ref)  -> É a POSIÇÃO na fita (onde você está)
    // (**ptr_ref) -> É o VALOR no quadrado (o número de 0 a 255)
    // (*total)    -> É o NÚMERO de variáveis na agenda

    if (strcmp(comando, "MAIS") == 0) {
        (**ptr_ref)++; // Soma no VALOR
    }  else if (strcmp(comando, "PULAR") == 0) {
        char *nome_alvo = strtok(NULL, " \n\r");
        if  (nome_alvo && arquivo_global) {
            long endereco = buscar_linha_do_rotulo(nome_alvo);
            if (endereco != -1) {
                fseek(arquivo_global, endereco, SEEK_SET);
            }
        }
    }
    else if (strcmp(comando, "SOMA") == 0 || strcmp(comando, "SOMAR") == 0) {
        char *valor_texto = strtok(NULL, " \n\r");
        if (valor_texto) (**ptr_ref) += atoi(valor_texto);
    } 
    else if (strcmp(comando, "SUBTRAIR") == 0 || strcmp(comando, "SUBTRAI") == 0) {
        char *valor_texto = strtok(NULL, " \n\r");
        if (valor_texto) (**ptr_ref) -= atoi(valor_texto);
    } 
    else if (strcmp(comando, "ZERAR") == 0) {
        **ptr_ref = 0;
    } 
    else if (strcmp(comando, "IR") == 0) {
        char *alvo = strtok(NULL, " \n\r");
        int posicao = -1;

        // Procura na agenda usando (*total)
        for (int i = 0; i < *total; i++) {
            if (strcmp(agenda[i].nome, alvo) == 0) {
                posicao = agenda[i].endereco;
                break;
            }
        }
        
        if (posicao == -1 && alvo) posicao = atoi(alvo);

        if (posicao >= 0 && posicao < 30000) {
            *ptr_ref = &tapebase[posicao]; // Muda a POSIÇÃO oficial
        }
    }
    else if (strcmp(comando, "MENOS") == 0) {
        (**ptr_ref)--;
    } 
    else if (strcmp(comando, "ENTRADA") == 0) {
        int temp;
        printf("\n Digite um numero (0-255): ");
        
        if (scanf(" %d", &temp) == 1) {
            **ptr_ref = (unsigned char)(temp & 0xFF);
            printf(" Valor %d armazenado. \n", **ptr_ref);
        } else {
            printf(" Entrada invalida.\n");
        }
        while (getchar() != '\n' && getchar() != EOF);
    }
    else if (strcmp(comando, "DIREITA") == 0) {
        if (*ptr_ref < tapebase + 29999) {
            (*ptr_ref)++; // Move a POSIÇÃO para frente
        }
    }
    else if (strcmp(comando, "ESQUERDA") == 0) {
        if (*ptr_ref > tapebase) {
            (*ptr_ref)--; // Move a POSIÇÃO para trás
        }
    } 
    else if (strcmp(comando, "VALOR") == 0) {
        printf("%d \n", **ptr_ref); // Imprime o VALOR
        fflush(stdout);
    } 
    else if (strcmp(comando, "NOMEAR") == 0) {
        char *pos_str = strtok(NULL, " \n\r");
        char *nome_str = strtok(NULL, " \n\r");

        if (pos_str && nome_str) {
            agenda[*total].endereco = atoi(pos_str);
            strcpy(agenda[*total].nome, nome_str);
            (*total)++; // Incrementa o contador oficial
            printf("[AXIS] %s agora e o quadrado %s\n", nome_str, pos_str);
        }
    }
    else if (strcmp(comando, "FALAR") == 0) {
        putchar(**ptr_ref); // Imprime o caractere (VALOR)
        fflush(stdout);
    }
}

void interpretar_linha(char *linha, unsigned char **ptr_ref, unsigned char *tapebase, Variavel *agenda, int *total) {
    // suporte a comentario
    char *comentario = strstr(linha, "$");
    if (comentario) {
        *comentario = '\0';
    }

    char linha_original[500];
    strcpy(linha_original, linha);

    char *comando = strtok(linha, " \n\r");
    while (comando != NULL)
    {
        if (strcmp(comando, "REPETE") == 0) {
            char *vezes_str = strtok(NULL, " \n\r");
            int vezes = (vezes_str) ? atoi(vezes_str) : 0;

            char *inicio = strchr(linha_original, '[');
            char *fim = strrchr(linha_original, ']');

            if (inicio && fim && fim > inicio) {
                *fim = '\0';
                char *bloco = inicio + 1;
                for (int i = 0; i < vezes; i++)
                {
                    char copia_bloco[256];
                    strcpy(copia_bloco, bloco);
                    char *sub = strtok(copia_bloco, " \n\r");
                    while (sub != NULL)
                    {
                        executar(sub, ptr_ref, tapebase, agenda, total);
                        sub = strtok(NULL, " \n\r");
                    }
                    
                }
                int offset = (int)(fim - linha_original) + 1;
                comando = strtok(linha + offset, " \n\r");
                continue;
                }
            }
            else if (strcmp(comando, "CARREGAR") == 0) {
                char *nome_arquivo = strtok(NULL, " \n\r");
                if (nome_arquivo) {
                    FILE *arq = fopen(nome_arquivo, "r");
                    if (arq) {
                        char linha_arq[500];
                        while (fgets(linha_arq, sizeof(linha_arq), arq)) {
                            interpretar_linha(linha_arq, ptr_ref, tapebase, agenda, total);
                        }
                        fclose(arq);
                    } else {
                        printf("[ERRO] Arquivo %s nao encontrado.\n", nome_arquivo);
                    }
                }
            }
            else {
                executar(comando, ptr_ref, tapebase, agenda, total);
            }
            comando = strtok(NULL, " \n\r");
        }
    }


int main(int argc, char*argv[]) {
    char code[500];
    Variavel agenda[50];
    int total_nomes = 0;
    unsigned char tape[30000] = {0};
    unsigned char *ptr = tape;

    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        printf("[ERRO]");
        return 1;
    }
    arquivo_global = f; // <--- AQUI A MÁGICA ACONTECE!

    mapear_rotulo(arquivo_global);
    rewind(arquivo_global);

    char linha[500];
    while (fgets(linha, sizeof(linha), arquivo_global)) {
        interpretar_linha(linha, &ptr, tape, agenda, &total_nomes);
    }

    fclose(f);
    return 0;
}