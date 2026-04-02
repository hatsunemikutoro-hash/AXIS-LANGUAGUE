#include "axis.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: axis <arquivo.axis> [-v]\n");
        fprintf(stderr, "  -v   Modo debug (verbose)\n");
        return 1;
    }

    EstadoAxis estado;
    axis_init(&estado);

    // Verifica flag -v para modo verbose/debug
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0)
            estado.verbose = 1;
    }

    return axis_executar_arquivo(&estado, argv[1]);
}
