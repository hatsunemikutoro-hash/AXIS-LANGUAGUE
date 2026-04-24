#define _POSIX_C_SOURCE 200809L
#include "axis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// =============================================================================
//  Internal Types
// =============================================================================

typedef struct {
    AxisState *state;
    char     **saveptr;
} Ctx;

typedef void (*CommandFn)(Ctx *ctx);

typedef struct {
    const char *name;
    CommandFn   fn;
} CommandEntry;

// =============================================================================
//  Internal Utilities
// =============================================================================

static inline char *next_token(Ctx *ctx) {
    return strtok_r(NULL, " \t\n\r", ctx->saveptr);
}

static void debug_print(AxisState *s, const char *cmd) {
    if (!s->verbose) return;
    printf("[DBG] CMD: %-12s | PTR=%04d | VAL=%03d | DEPTH=%d\n",
           cmd,
           (int)(s->ptr - s->tape),
           (int)*(s->ptr),
           s->call_depth);
    fflush(stdout);
}

// -----------------------------------------------------------------------------
//  Lookup Helpers
// -----------------------------------------------------------------------------

static long find_label(AxisState *s, const char *name) {
    for (int i = 0; i < s->total_labels; i++) {
        if (strcmp(s->labels[i].name, name) == 0)
            return s->labels[i].position;
    }
    fprintf(stderr, "AXIS ERROR: Label '%s' not found!\n", name);
    return -1;
}

static int find_variable(AxisState *s, const char *name) {
    for (int i = s->total_vars - 1; i >= 0; i--) {
        if (strcmp(s->vars[i].name, name) == 0)
            return s->vars[i].address;
    }
    return -1;
}

static Function *find_function(AxisState *s, const char *name) {
    for (int i = 0; i < s->total_funcs; i++) {
        if (strcmp(s->funcs[i].name, name) == 0)
            return &s->funcs[i];
    }
    return NULL;
}

static void jump_to_label(AxisState *s, const char *name) {
    long pos = find_label(s, name);
    if (pos != -1 && s->file)
        fseek(s->file, pos, SEEK_SET);
}

static unsigned char resolve_value(AxisState *s, const char *token) {
    int addr = find_variable(s, token);
    if (addr != -1)
        return s->tape[addr];
    return (unsigned char)atoi(token);
}

static int resolve_address(AxisState *s, const char *token) {
    int addr = find_variable(s, token);
    if (addr != -1)
        return addr;
    return atoi(token);
}

static int alloc_temp(AxisState *s) {
    if (s->next_temp < (TAPE_SIZE / 2)) {
        fprintf(stderr, "AXIS ERROR: Parameter memory exhausted!\n");
        return -1;
    }
    return s->next_temp--;
}

// =============================================================================
//  Command Implementations
// =============================================================================

// --- Arithmetic --------------------------------------------------------------

static void cmd_inc(Ctx *ctx)   { (*ctx->state->ptr)++; }
static void cmd_dec(Ctx *ctx)   { (*ctx->state->ptr)--; }
static void cmd_clear(Ctx *ctx) { *ctx->state->ptr = 0; }

static void cmd_add(Ctx *ctx) {
    char *arg = next_token(ctx);
    if (!arg) return;
    *ctx->state->ptr += resolve_value(ctx->state, arg);
}

static void cmd_sub(Ctx *ctx) {
    char *arg = next_token(ctx);
    if (!arg) return;
    *ctx->state->ptr -= resolve_value(ctx->state, arg);
}

// --- Tape Navigation ---------------------------------------------------------

static void cmd_next(Ctx *ctx) {
    AxisState *s = ctx->state;
    if (s->ptr < s->tape + TAPE_SIZE - 1) s->ptr++;
    else fprintf(stderr, "AXIS WARNING: Right tape boundary reached!\n");
}

static void cmd_prev(Ctx *ctx) {
    AxisState *s = ctx->state;
    if (s->ptr > s->tape) s->ptr--;
    else fprintf(stderr, "AXIS WARNING: Left tape boundary reached!\n");
}

static void cmd_seek(Ctx *ctx) {
    AxisState *s = ctx->state;
    char *target = next_token(ctx);
    if (!target) return;
    int pos = resolve_address(s, target);
    if (pos >= 0 && pos < TAPE_SIZE)
        s->ptr = &s->tape[pos];
    else
        fprintf(stderr, "AXIS ERROR: Invalid position in SEEK: %d\n", pos);
}

// --- Memory Operations -------------------------------------------------------

static void cmd_copy(Ctx *ctx) {
    AxisState *s = ctx->state;
    char *from_str = next_token(ctx);
    char *to_str   = next_token(ctx);
    if (!from_str || !to_str) {
        fprintf(stderr, "AXIS ERROR: COPY requires <from> <to>\n");
        return;
    }
    int from = resolve_address(s, from_str);
    int to   = resolve_address(s, to_str);
    if (from < 0 || from >= TAPE_SIZE || to < 0 || to >= TAPE_SIZE) {
        fprintf(stderr, "AXIS ERROR: Position out of bounds in COPY\n");
        return;
    }
    s->tape[to] = s->tape[from];
}

// --- Variable Management -----------------------------------------------------

static void cmd_let(Ctx *ctx) {
    AxisState *s   = ctx->state;
    char *pos_str  = next_token(ctx);
    char *name_str = next_token(ctx);
    if (!pos_str || !name_str) {
        fprintf(stderr, "AXIS ERROR: LET requires <position> <name>\n");
        return;
    }
    if (s->total_vars >= MAX_VARS) {
        fprintf(stderr, "AXIS ERROR: Variable limit reached!\n");
        return;
    }
    s->vars[s->total_vars].address = atoi(pos_str);
    strncpy(s->vars[s->total_vars].name, name_str, MAX_NAME - 1);
    s->vars[s->total_vars].name[MAX_NAME - 1] = '\0';
    s->total_vars++;
}

// --- Control Flow ------------------------------------------------------------

static void cmd_mark(Ctx *ctx) {
    next_token(ctx); // label name consumed during pre-scan; skip at runtime
}

static void cmd_jump(Ctx *ctx) {
    char *target = next_token(ctx);
    if (target) jump_to_label(ctx->state, target);
}

static void cmd_if_eq(Ctx *ctx) {
    char *val_str = next_token(ctx);
    char *target  = next_token(ctx);
    if (!val_str || !target) return;
    if (*ctx->state->ptr == resolve_value(ctx->state, val_str))
        jump_to_label(ctx->state, target);
}

static void cmd_if_lt(Ctx *ctx) {
    char *val_str = next_token(ctx);
    char *target  = next_token(ctx);
    if (!val_str || !target) return;
    if (*ctx->state->ptr < resolve_value(ctx->state, val_str))
        jump_to_label(ctx->state, target);
}

static void cmd_if_gt(Ctx *ctx) {
    char *val_str = next_token(ctx);
    char *target  = next_token(ctx);
    if (!val_str || !target) return;
    if (*ctx->state->ptr > resolve_value(ctx->state, val_str))
        jump_to_label(ctx->state, target);
}

// --- Input / Output ----------------------------------------------------------

static void cmd_read(Ctx *ctx) {
    int temp;
    printf("Enter a number (0-255): ");
    fflush(stdout);
    if (scanf("%d", &temp) == 1) {
        *ctx->state->ptr = (unsigned char)(temp & 0xFF);
    } else {
        fprintf(stderr, "AXIS ERROR: Invalid input.\n");
        int c; while ((c = getchar()) != '\n' && c != EOF);
    }
    int c; while ((c = getchar()) != '\n' && c != EOF);
}

static void cmd_print(Ctx *ctx) {
    printf("%d\n", (int)*ctx->state->ptr);
    fflush(stdout);
}

static void cmd_say(Ctx *ctx) {
    char *arg = strtok_r(NULL, "", ctx->saveptr);
    if (arg) {
        while (*arg == ' ' || *arg == '\t') arg++;
        if (arg[0] == '"') {
            char *end = strrchr(arg, '"');
            if (end > arg) {
                for (char *p = arg + 1; p < end; p++)
                    putchar(*p);
                putchar('\n');
                fflush(stdout);
                return;
            }
        }
    }
    printf("%c", *ctx->state->ptr);
    fflush(stdout);
}

// --- Functions ---------------------------------------------------------------

static void cmd_def(Ctx *ctx) {
    // Consumed at pre-scan time; skip all tokens at runtime
    char *tok;
    while ((tok = next_token(ctx)) != NULL) (void)tok;
}

static void cmd_return(Ctx *ctx);  // forward declaration

static void cmd_end(Ctx *ctx) {
    if (ctx->state->call_depth > 0)
        cmd_return(ctx);
}

static void cmd_call(Ctx *ctx) {
    AxisState *s = ctx->state;

    char *func_name = next_token(ctx);
    if (!func_name) {
        fprintf(stderr, "AXIS ERROR: CALL requires a function name\n");
        return;
    }

    Function *fn = find_function(s, func_name);
    if (!fn) {
        fprintf(stderr, "AXIS ERROR: Function '%s' not found!\n", func_name);
        return;
    }

    if (s->call_depth >= MAX_CALL_STACK) {
        fprintf(stderr, "AXIS ERROR: Recursion limit reached!\n");
        return;
    }

    // Resolve all arguments before touching the call stack
    unsigned char values[MAX_PARAMS];
    for (int i = 0; i < fn->total_params; i++) {
        char *arg = next_token(ctx);
        if (!arg) {
            fprintf(stderr, "AXIS ERROR: Function '%s' expects %d arg(s), missing arg %d\n",
                    func_name, fn->total_params, i + 1);
            return;
        }
        values[i] = resolve_value(s, arg);
    }

    // Save caller context
    s->call_stack[s->call_depth]      = ftell(s->file);
    s->vars_on_enter[s->call_depth]   = s->total_vars;
    s->ptr_on_enter[s->call_depth]    = s->ptr;
    s->call_depth++;

    // Bind parameters as temporary variables
    for (int i = 0; i < fn->total_params; i++) {
        int cell = alloc_temp(s);
        if (cell == -1) { s->call_depth--; return; }
        s->tape[cell] = values[i];
        if (s->total_vars < MAX_VARS) {
            s->vars[s->total_vars].address = cell;
            strncpy(s->vars[s->total_vars].name, fn->params[i], MAX_NAME - 1);
            s->vars[s->total_vars].name[MAX_NAME - 1] = '\0';
            s->total_vars++;
        }
    }

    if (s->file)
        fseek(s->file, fn->position, SEEK_SET);

    if (s->verbose)
        printf("[DBG] CALL '%s' with %d arg(s) | depth=%d\n",
               func_name, fn->total_params, s->call_depth);
}

static void cmd_return(Ctx *ctx) {
    AxisState *s = ctx->state;

    if (s->call_depth <= 0) {
        fprintf(stderr, "AXIS WARNING: RETURN outside a function, ignored.\n");
        return;
    }

    s->call_depth--;
    s->total_vars = s->vars_on_enter[s->call_depth];
    s->ptr        = s->ptr_on_enter[s->call_depth];

    if (s->file)
        fseek(s->file, s->call_stack[s->call_depth], SEEK_SET);

    if (s->verbose)
        printf("[DBG] RETURN | depth now=%d\n", s->call_depth);
}

// --- Modules -----------------------------------------------------------------

// LOAD and REPEAT are handled inline in axis_interpret_line()
// These stubs exist so the dispatch table can reference them.
static void cmd_load(Ctx *ctx)   { (void)ctx; }
static void cmd_repeat(Ctx *ctx) { (void)ctx; }

// --- System / Debug ----------------------------------------------------------

static void cmd_exit(Ctx *ctx) { (void)ctx; exit(0); }

static void cmd_debug(Ctx *ctx) {
    ctx->state->verbose = 1;
    printf("[DBG] Debug mode enabled.\n");
}

static void cmd_nodebug(Ctx *ctx) {
    ctx->state->verbose = 0;
}

// =============================================================================
//  Dispatch Table
// =============================================================================

static const CommandEntry COMMANDS[] = {
    // Arithmetic
    {"INC",     cmd_inc},
    {"DEC",     cmd_dec},
    {"CLEAR",   cmd_clear},
    {"ADD",     cmd_add},
    {"SUB",     cmd_sub},
    // Tape navigation
    {"NEXT",    cmd_next},
    {"PREV",    cmd_prev},
    {"SEEK",    cmd_seek},
    // Memory
    {"COPY",    cmd_copy},
    // Variables
    {"LET",     cmd_let},
    // Control flow
    {"MARK",    cmd_mark},
    {"JUMP",    cmd_jump},
    {"IF_EQ",   cmd_if_eq},
    {"IF_LT",   cmd_if_lt},
    {"IF_GT",   cmd_if_gt},
    // I/O
    {"READ",    cmd_read},
    {"PRINT",   cmd_print},
    {"SAY",     cmd_say},
    // Functions
    {"DEF",     cmd_def},
    {"END",     cmd_end},
    {"CALL",    cmd_call},
    {"RETURN",  cmd_return},
    // Modules
    {"LOAD",    cmd_load},
    {"REPEAT",  cmd_repeat},
    // System
    {"EXIT",    cmd_exit},
    {"DEBUG",   cmd_debug},
    {"NODEBUG", cmd_nodebug},
    {NULL, NULL}
};

static CommandFn find_command(const char *name) {
    for (int i = 0; COMMANDS[i].name != NULL; i++) {
        if (strcmp(COMMANDS[i].name, name) == 0)
            return COMMANDS[i].fn;
    }
    return NULL;
}

// =============================================================================
//  Pre-scan: Labels
// =============================================================================

void axis_scan_labels(AxisState *s) {
    char token[MAX_NAME];

    rewind(s->file);

    while (fscanf(s->file, "%63s", token) != EOF) {
        if (strcmp(token, "MARK") != 0) continue;

        if (s->total_labels >= MAX_LABELS) {
            fprintf(stderr, "AXIS ERROR: Label limit reached!\n");
            break;
        }
        if (fscanf(s->file, "%63s", token) != 1) break;

        // Consume the rest of the line so position lands on the next line
        { int c; while ((c = fgetc(s->file)) != '\n' && c != EOF); }

        snprintf(s->labels[s->total_labels].name, MAX_NAME, "%s", token);
        s->labels[s->total_labels].position = ftell(s->file);
        s->total_labels++;
    }
}

// =============================================================================
//  Pre-scan: Functions
// =============================================================================

void axis_scan_functions(AxisState *s) {
    char line[MAX_LINE];

    rewind(s->file);

    while (fgets(line, sizeof(line), s->file)) {
        // Strip comments
        char *comment = strchr(line, '$');
        if (comment) *comment = '\0';

        char *saveptr = NULL;
        char *token = strtok_r(line, " \t\n\r", &saveptr);
        if (!token || strcmp(token, "DEF") != 0) continue;

        if (s->total_funcs >= MAX_FUNCS) {
            fprintf(stderr, "AXIS ERROR: Function limit reached!\n");
            break;
        }

        char *name = strtok_r(NULL, " \t\n\r", &saveptr);
        if (!name) continue;

        Function *fn = &s->funcs[s->total_funcs];
        strncpy(fn->name, name, MAX_NAME - 1);
        fn->name[MAX_NAME - 1] = '\0';
        fn->total_params = 0;

        char *param;
        while ((param = strtok_r(NULL, " \t\n\r", &saveptr)) != NULL) {
            if (fn->total_params >= MAX_PARAMS) {
                fprintf(stderr, "AXIS WARNING: Function '%s' exceeds %d params, extras ignored\n",
                        fn->name, MAX_PARAMS);
                break;
            }
            strncpy(fn->params[fn->total_params], param, MAX_NAME - 1);
            fn->params[fn->total_params][MAX_NAME - 1] = '\0';
            fn->total_params++;
        }

        fn->position = ftell(s->file);
        s->total_funcs++;
    }
}

// =============================================================================
//  Line Interpreter
// =============================================================================

void axis_interpret_line(AxisState *s, char *line) {
    // Normalize Windows line endings
    char *cr = strchr(line, '\r');
    if (cr) *cr = '\n';

    // Strip comments
    char *comment = strchr(line, '$');
    if (comment) *comment = '\0';

    // Skip blank / whitespace-only lines
    for (char *p = line; *p; p++) {
        if (!isspace((unsigned char)*p)) goto not_blank;
    }
    return;
not_blank:;

    // Keep a copy of the original line for REPEAT block parsing
    char original[MAX_LINE];
    strncpy(original, line, MAX_LINE - 1);
    original[MAX_LINE - 1] = '\0';

    char *saveptr = NULL;
    char *token = strtok_r(line, " \t\n\r", &saveptr);

    while (token != NULL) {

        // ── REPEAT ──────────────────────────────────────────────────────────
        if (strcmp(token, "REPEAT") == 0) {
            char *times_str = strtok_r(NULL, " \t\n\r", &saveptr);
            int   times     = times_str ? atoi(times_str) : 0;
            char *block_start = strchr(original, '[');
            char *block_end   = strrchr(original, ']');

            if (block_start && block_end && block_end > block_start) {
                *block_end = '\0';
                char *block = block_start + 1;

                for (int i = 0; i < times; i++) {
                    char copy[MAX_LINE];
                    strncpy(copy, block, MAX_LINE - 1);
                    copy[MAX_LINE - 1] = '\0';

                    char *sp2 = NULL;
                    char *sub = strtok_r(copy, " \t\n\r", &sp2);
                    while (sub) {
                        Ctx inner = {s, &sp2};
                        CommandFn fn = find_command(sub);
                        if (fn) { debug_print(s, sub); fn(&inner); }
                        else fprintf(stderr, "AXIS WARNING: Unknown command in REPEAT: '%s'\n", sub);
                        sub = strtok_r(NULL, " \t\n\r", &sp2);
                    }
                }
            }
            return; // REPEAT always consumes the rest of the line
        }

        // ── LOAD ────────────────────────────────────────────────────────────
        if (strcmp(token, "LOAD") == 0) {
            char *filename = strtok_r(NULL, " \t\n\r", &saveptr);
            if (!filename) return;

            FILE *prev_file = s->file;
            FILE *f = fopen(filename, "r");
            if (!f) {
                fprintf(stderr, "AXIS ERROR: Could not open '%s'\n", filename);
                return;
            }

            s->file = f;
            axis_scan_labels(s);
            axis_scan_functions(s);
            rewind(f);

            char sub_line[MAX_LINE];
            while (fgets(sub_line, sizeof(sub_line), f))
                axis_interpret_line(s, sub_line);

            fclose(f);
            s->file = prev_file;
            return;
        }

        // ── Normal command dispatch ──────────────────────────────────────────
        Ctx ctx = {s, &saveptr};
        CommandFn fn = find_command(token);

        if (fn) {
            debug_print(s, token);
            fn(&ctx);

            // These commands consume the rest of the line themselves
            if (strcmp(token, "SAY")    == 0 ||
                strcmp(token, "JUMP")   == 0 ||
                strcmp(token, "IF_EQ")  == 0 ||
                strcmp(token, "IF_LT")  == 0 ||
                strcmp(token, "IF_GT")  == 0 ||
                strcmp(token, "RETURN") == 0 ||
                strcmp(token, "END")    == 0 ||
                strcmp(token, "DEF")    == 0) {
                return;
            }
        } else {
            fprintf(stderr, "AXIS WARNING: Unknown command: '%s'\n", token);
        }

        token = strtok_r(NULL, " \t\n\r", &saveptr);
    }
}

// =============================================================================
//  Initialization and Execution
// =============================================================================

void axis_init(AxisState *s) {
    memset(s, 0, sizeof(*s));
    s->ptr       = s->tape;
    s->next_temp = TAPE_SIZE - 1;
}

int axis_run_file(AxisState *s, const char *path) {
    s->file = fopen(path, "rb");
    if (!s->file) {
        fprintf(stderr, "AXIS ERROR: Could not open '%s'\n", path);
        return -1;
    }

    // Reject UTF-16 encoded files (BOM: FF FE or FE FF)
    {
        unsigned char bom[2] = {0};
        if (fread(bom, 1, 2, s->file) == 2 &&
            ((bom[0] == 0xFF && bom[1] == 0xFE) ||
             (bom[0] == 0xFE && bom[1] == 0xFF))) {
            fprintf(stderr, "AXIS ERROR: File is UTF-16. Save as UTF-8 without BOM.\n");
            fclose(s->file);
            s->file = NULL;
            return -1;
        }
        rewind(s->file);
    }

    axis_scan_labels(s);
    axis_scan_functions(s);
    rewind(s->file);

    // Skip UTF-8 BOM (EF BB BF) if present
    {
        unsigned char bom[3] = {0};
        if (fread(bom, 1, 3, s->file) == 3 &&
            bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) {
            // BOM consumed — continue from current position
        } else {
            rewind(s->file);
        }
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), s->file)) {
        // Skip function bodies at the top level (call_depth == 0)
        // They are only executed via CALL.
        char copy[MAX_LINE];
        strncpy(copy, line, MAX_LINE - 1);
        copy[MAX_LINE - 1] = '\0';

        char *comment = strchr(copy, '$');
        if (comment) *comment = '\0';

        char *saveptr = NULL;
        char *first = strtok_r(copy, " \t\n\r", &saveptr);
        if (first && strcmp(first, "DEF") == 0 && s->call_depth == 0) {
            // Fast-forward past the function body until END
            while (fgets(line, sizeof(line), s->file)) {
                char *c2 = strchr(line, '$');
                if (c2) *c2 = '\0';
                char *p = line;
                while (*p == ' ' || *p == '\t') p++;
                if (strncmp(p, "END", 3) == 0) break;
            }
            continue;
        }

        axis_interpret_line(s, line);
        // Note: if axis_interpret_line performed a fseek (e.g. via CALL/RETURN),
        // fgets will correctly continue from the new file position.
    }

    fclose(s->file);
    s->file = NULL;
    return 0;
}