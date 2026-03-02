#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "axis.h"


FILE *arquivo_global = NULL; // A GAMBIARRA DOS DEUSES
Rotulo mapa_de_rotulos[100]; 
int total_rotulos = 0;

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