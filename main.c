#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct
{
    char nome[20];
    int endereco;
} Variavel;


void executar(char *comando, unsigned char **ptr_ref, unsigned char *tapebase, Variavel *agenda, int *total) {
    // REGRAS PARA NÃO ERRAR MAIS:
    // (*ptr_ref)  -> É a POSIÇÃO na fita (onde você está)
    // (**ptr_ref) -> É o VALOR no quadrado (o número de 0 a 255)
    // (*total)    -> É o NÚMERO de variáveis na agenda

    if (strcmp(comando, "MAIS") == 0) {
        (**ptr_ref)++; // Soma no VALOR
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
        printf("%d", **ptr_ref); // Imprime o VALOR
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

int main() {
    char code[500];
    Variavel agenda[50];
    int total_nomes = 0;
    unsigned char tape[30000] = {0};
    unsigned char *ptr = tape;

    printf("--- AXIS INTERPRETER v1.0 ---\n");
    printf("------------------------------------\n\n");

    while (1) {
        printf("\n>> ");
        if (fgets(code, sizeof(code), stdin) == NULL) break;

        char linha_original[500];
        strcpy(linha_original, code);

        char *comando = strtok(code, " \n\r");
        
        while (comando != NULL) {
            if (strcmp(comando, "REPETE") == 0) {
                char *vezes_str = strtok(NULL, " \n\r");
                int vezes = (vezes_str) ? atoi(vezes_str) : 0;

                char *inicio = strchr(linha_original, '[');
                char *fim = strrchr(linha_original, ']');

                if (inicio && fim && fim > inicio) {
                    *fim = '\0'; 
                    char *bloco = inicio + 1; 

                    for (int i = 0; i < vezes; i++) {
                        char copia_bloco[256];
                        strcpy(copia_bloco, bloco);
                        char *sub = strtok(copia_bloco, " \n\r");
                        while (sub != NULL) {
                            executar(sub, &ptr, tape, agenda, &total_nomes);
                            sub = strtok(NULL, " \n\r");
                        }
                    }
                    int offset = (int)(fim - linha_original) + 1;
                    comando = strtok(code + offset, " \n\r");
                    continue; 
                }
            } else {
                executar(comando, &ptr, tape, agenda, &total_nomes);
            }
            comando = strtok(NULL, " \n\r");
        }
    } 
    return 0;
}