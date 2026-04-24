#ifndef AXIS_H
#define AXIS_H

#include <stdio.h>

// =============================================================================
//  Constants
// =============================================================================

#define TAPE_SIZE      30000
#define MAX_LABELS     512
#define MAX_VARS       512
#define MAX_FUNCS      128
#define MAX_PARAMS     16
#define MAX_CALL_STACK 64
#define MAX_NAME       64
#define MAX_LINE       512

// =============================================================================
//  Structures
// =============================================================================

typedef struct {
    char name[MAX_NAME];
    long position;      // file offset immediately after the MARK line
} Label;

typedef struct {
    char name[MAX_NAME];
    int  address;
} Variable;

typedef struct {
    char name[MAX_NAME];
    char params[MAX_PARAMS][MAX_NAME]; // parameter names
    int  total_params;
    long position;      // file offset immediately after the DEF line
} Function;

// =============================================================================
//  Interpreter State
// =============================================================================

typedef struct {
    unsigned char  tape[TAPE_SIZE];
    unsigned char *ptr;             // current tape pointer

    Label    labels[MAX_LABELS];
    int      total_labels;

    Variable vars[MAX_VARS];
    int      total_vars;

    Function funcs[MAX_FUNCS];
    int      total_funcs;

    // Return stack: stores the file position for each active CALL
    long           call_stack[MAX_CALL_STACK];
    // Number of variables that existed when each call frame was entered,
    // so they can be unwound on RETURN
    int            vars_on_enter[MAX_CALL_STACK];
    // Tape pointer at the time of each call (restored on RETURN)
    unsigned char *ptr_on_enter[MAX_CALL_STACK];
    // Temp allocator position at the time of each call (restored on RETURN)
    int            next_temp_on_enter[MAX_CALL_STACK]; // NEW
    int            call_depth;      // current call stack depth

    // Next free cell for temporary parameter/VAR allocation.
    // Starts at the end of the tape and grows backward,
    // away from the user-accessible region.
    int   next_temp;

    FILE *file;
    int   verbose;                  // debug mode (-v flag)
} AxisState;

// =============================================================================
//  Public API
// =============================================================================

void axis_init(AxisState *s);
void axis_scan_labels(AxisState *s);
void axis_scan_functions(AxisState *s);
int  axis_run_file(AxisState *s, const char *path);
void axis_interpret_line(AxisState *s, char *line);

#endif // AXIS_H