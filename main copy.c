#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct
{
    char nome[20];
    int endereco;
} Variavel;


int main() {
    char code[500];
    Variavel agenda[50];
    int total_nomes = 0;
    unsigned char tape[30000] = {0};
    unsigned char *ptr = tape;

    printf("--- AXIS INTERPRETER v1.0 ---\n");
    printf("------------------------------------\n\n");
    printf(">> ");
    // Usar fgets é melhor para evitar que espaços quebrem o código
    if (fgets(code, sizeof(code), stdin) == NULL)  return 0;;

    char *comando = strtok(code, " \n\r");

    while (comando != NULL) {
        if (strcmp(comando, "MAIS") == 0) {
            (*ptr)++;
        } else if (strcmp(comando, "SOMA") == 0 || strcmp(comando, "SOMAR") == 0) {
            char *valor_texto = strtok(NULL, " \n\r");
            if (valor_texto != NULL) {
                int valor = atoi(valor_texto);
                *ptr += valor;
            }
        } else if (strcmp(comando, "SUBTRAIR") == 0 || strcmp(comando, "SUBTRAI") == 0) {
            char *valor_texto = strtok(NULL, " \n\r");
            if (valor_texto != NULL) {
                int valor = atoi(valor_texto);
                *ptr -= valor;
            }
        } else if (strcmp(comando, "ZERAR") == 0) {
                *ptr = 0;
            } else if (strcmp(comando, "IR") == 0) {
                char *alvo = strtok(NULL, " \n\r");
                int posicao = -1;

                for (int i = 0; i < total_nomes; i++)
                {
                    if (strcmp(agenda[i].nome, alvo) == 0) {
                        posicao = agenda[i].endereco;
                        break;
                    }
                }
                
                if (posicao == -1) posicao = atoi(alvo);

                if  (posicao > 0 && posicao < 30000) {
                    ptr = &tape[posicao];
                }

            }
        else if (strcmp(comando, "MENOS") == 0) {
            (*ptr)--;
        }
        else if (strcmp(comando, "DIREITA") == 0) {
            if (ptr < tape + 29999) {
                ptr++;
            }
            
        }
        else if (strcmp(comando, "ESQUERDA") == 0) {
            if (ptr > tape) {
                ptr--;
            }
        } else if (strcmp(comando, "VALOR") == 0) {
            printf("%d", *ptr);
            fflush(stdout);
        } else if (strcmp(comando, "NOMEAR") == 0) {
            char *pos_str = strtok(NULL, " \n\r");
            char *nome_str = strtok(NULL, " \n\r");

            if (pos_str && nome_str) {
                agenda[total_nomes].endereco = atoi(pos_str);
                strcpy(agenda[total_nomes].nome, nome_str);
                total_nomes ++;
                printf("[AXIS] %s agora e o quadrado %s\n", nome_str, pos_str);
            }
        }
        else if (strcmp(comando, "FALAR") == 0) {
            putchar(*ptr);
            fflush(stdout);
        }
        comando = strtok(NULL, " \n\r");
    }
    return 0;
}