#include "axis.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: axis <file.axis> [-v]\n");
        fprintf(stderr, "  -v   Debug mode (verbose)\n");
        return 1;
    }

    AxisState state;
    axis_init(&state);

    // Check for -v flag to enable verbose/debug mode
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0)
            state.verbose = 1;
    }

    return axis_run_file(&state, argv[1]);
}