#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "scar.h"
#include "lexer.h"
#include "parser.h"
#include "eval.h"

#include "timing.h"

/* For now, grammar is as follows:
Top      	-> Prog
Prog     	-> FunDecs
FunDecs  	-> FunDec FunDecs
FunDecs  	-> FunDec
FunDec 		-> Type Id ( FunArgs = BodyStats
BodyStats	-> Stats BodyStats
BodyStats	-> if ( Exps ) OuterThen else OuterElse
BodyStats	-> return RetVal
OuterThen	-> Stat OuterThen
OuterThen	-> If ( Exps ) Then else ElseBranch OuterThen
OuterThen	-> return RetVal
OuterElse	-> Stat OuterElse
OuterElse	-> if ( Exps ) Then else ElseBranch OuterElse
OuterElse	-> return Retval
Then	    -> Stat Then
Then     	-> Stat
Then     	-> if ( Exps ) Then else ElseBranch
Then     	-> return RetVal
ElseBranch	-> Stat ElseBranch
ElseBranch	-> constate->tinue ;
ElseBranch	-> return RetVal
Type 		-> int
Type 		-> float
Type 		-> void
FunArgs 	-> FunArg FunArgs
FunArgs  	-> Type Id )
FunArgs 	-> )
FunArg      -> Type Id ,
Stats 		-> Stat Stats
Stats 		-> Stat
Stat     	-> Type Id = Exps ;
Stat        -> FunCall ;
Stat        -> while ( Exps ) Stats loop ;
Stat        -> if ( Exps ) Stats else ElsePart
ElsePart    -> Stats
ElsePart    -> proceed ;
FunCall     -> Id ( Params )
Params      -> Exps ParamNext
Params      ->
ParamNext   -> , Params
ParamNext   ->
Exps        -> ExpsA Truth ExpsA
Exps        -> ExpsA
ExpsA       -> ExpsB Comp ExpsB
ExpsB       -> ExpsC

ExpsA       -> ExpsB ExpsC
ExpsC       -> && ExpsD
ExpsC       ->
ExpsB        > ExpsE ExpsF
ExpsF       -> Comp ExpsG
ExpsF       ->
ExpsD       -> Exp
Exps

ExpsA ExpsB
ExpsB       -> Truth ExpsB
ExpsC        -> Exp Exp0
Exp0        -> Comp Exp1 Exp
Exp         -> Exp2 Exp1
Exp1        -> + Exp2 Exp1
Exp1        -> - Exp2 Exp1
Exp1        ->
Exp2        -> Exp4 Exp3
Exp3       	-> * Exp4 Exp3
Exp3       	-> / Exp4 Exp3
Exp3      	->
Exp4        -> Exp5 Exp4
Exp5        -> ** Exp4
Exp4        -> Val
Exp4        -> ( Exps )
Exp4        -> FunCall
Val			-> num
Val			-> id
Binop		-> +
Binop		-> -
Binop		-> *
Binop		-> /
Binop		-> **
Comp     	-> ==
Comp     	-> !=
Comp     	-> >
Comp     	-> <
Comp     	-> >=
Comp     	-> <=
RetVal   	-> Exps ;
RetVal   	-> ;
Truth       -> &&
Truth       -> ||
*/

/*
scratchpad:
Stat        -> Matched
Stat        -> Unmatched
Matched     -> if ( Exps ) Matched else Matched
Matched     -> Type Id = Exps ;
Matched     -> FunCall ;
Unmatched   -> if ( Exps ) Matched else Unmatched
Unmatched   -> if ( Exps ) Stats

Exps        -> Exp Comp Exp
Exps        -> Exp
Exp         -> Exp + Exp2
Exp         -> Exp - Exp2
Exp         -> Exp2
Exp3        -> Exp2 * Exp3
Exp2        -> Exp2 / Exp3
Exp2        -> Exp3
Exp3        -> ( Exp )
Exp3        -> FunCall
Exp3		-> Val

*/
/*
 * Still need to implement:
 * if...else...
 * while( Exps ) Stats loop
 * print()
 * command line arguments
 * && and ||
 * << and >>
 * ++ and --
 * +=, -=, *=, and /=
 * !
 * ^
 * %
 * "..."
 * '.'
 * arrays
 * tuples
 * sqrt()
 * typeof()
 * proper control flow scopes concerning variable declarations vs. updates within if/else bodies.
*/

void node_as_text(int n_index, FILE* output, struct state* st) {
    if (output == NULL) {
        output = stdout;
    }
    switch (st->tree->ns[n_index].node_type) {
        case n_stat:
            switch (st->tree->ns[n_index].specific_type) {
                case s_var_bind:
                    fprintf(output, "Statement: Variable bind");
                    return;
                case s_fun_bind:
                    fprintf(output, "Statement: Function bind");
                    return;
                case s_function_call:
                    fprintf(output, "Statement: Function call");
                    return;
                case s_if:
                    fprintf(output, "Statement: If");
                    return;
                case s_else:
                    fprintf(output, "Statement: Else");
                    return;
                case s_return:
                    fprintf(output, "Statement: Return");
                    return;
                case s_fun_body:
                    fprintf(output, "Statement: Function body");
                    return;
                case s_proceed:
                    fprintf(output, "Statement: Proceed");
                    return;
                default:
                    fprintf(output, "Unknown statement node");
                    return;
            }
        case n_expr:
            switch (st->tree->ns[n_index].specific_type) {
                case e_id:
                    fprintf(output, "Expression: Variable name");
                    return;
                case e_val:
                    fprintf(output, "Expression: Value");
                    return;
                case e_binop:
                    fprintf(output, "Expression: Binary operation");
                    return;
                case e_param:
                    fprintf(output, "Expression: Parameter declaration");
                    return;
                case e_comp:
                    fprintf(output, "Expression: Comparison operation");
                    return;
                case e_argument:
                    fprintf(output, "Expression: Function-call argument");
                    return;
                case e_funcall:
                    fprintf(output, "Expression: Function call");
                    return;
                case e_condition:
                    fprintf(output, "Expression: Condition");
                    return;
                default:
                    fprintf(output, "Unknown expression node");
                    return;
            }
        case n_prog:
            fprintf(output, "Top-level program");
            return;
        default:
            fprintf(output, "Unknown top-level program node");
            return;
    }
    return;
}

void print_node(int n_index, struct state* state) {
    node_as_text(n_index, stdout, state);
}

void fprint_node(int n_index, struct state* state) {
    node_as_text(n_index, state->output, state);
}

char* get_nval(int n_index, struct state* state) {
    if (state->tree->ns[n_index].token_count > 0)
        return state->ti->ts[state->tree->ns[n_index].token_indices[0]].val;
    return "<unkown>";
}

char* get_tval(int t_index, struct token_index* ti) {
    if (ti->n > t_index)
        return ti->ts[t_index].val;
    return "<unknown>";
}

void print_val(FILE* output, struct Val val) {
    switch (val.type) {
        case (int_val):
            fprintf(output, "%d", val.value.Int);
            return;
        case (float_val):
            fprintf(output, "%f", val.value.Float);
            return;
        case (char_arr):
            fprintf(output, "%s", val.value.String);
            return;
        case (bool_val):
            fprintf(output, "%d", val.value.Bool);
            return;
        default:
            fprintf(output, "%s", val.value.String);
            return;
    }
}

unsigned long current_line_no(struct state* st) {
    return st->ti->ts[st->i].line_number;
}

unsigned long current_char_no(struct state* st) {
    return st->ti->ts[st->i].char_number;
}

unsigned long line_no(int i, struct state* st) {
    return st->ti->ts[i].line_number;
}

unsigned long char_no(int i, struct state* st) {
    return st->ti->ts[i].char_number;
}

void free_tokens(struct token_index* ti) {
    if (!ti)
        return;
    for (int i = 0; i < ti->n; i++) {
        free(ti->ts[i].val);
    }
    free(ti->ts);
    free(ti);
}

void free_nodes(struct node_tree* nt) {
    if (!nt)
        return;
    for (int i = 0; i < nt->n; i++) {
        free(nt->ns[i].token_indices);
    }
    free(nt->ns);
    free(nt);
}

void free_ftable(struct ftable_index* ft) {
    if (!ft)
        return;
    if (ft->n > 0)
        free(ft->fs);
    free(ft);
}

void free_vtable(struct vtable_index* vt) {
    if (!vt)
        return;
    if (vt->n > 0)
        free(vt->vs);
    free(vt);
}

void free_ivtable(struct ivtable_index* vt) {
    if (!vt)
        return;
    if (vt->n > 0) {
        free(vt->vs);
    }
    free(vt);
}

int fail_input() {
    printf("Scar Compiler usage:\n./scar [option] <filename>\n");
    printf("Argument order irrelevant.\n");
    printf("Options (Mutually exclusive):\n  -v    Print in-depth log information.\n");
    printf("  -q    Hide all errors/warnings that would be printed. Result is still printed.\n");
    printf("  -t    Print timing information along with the result.\n");
    return 1;
}

int main(int argc, char* argv[]) {
    unsigned long before = microseconds();
    if (argc <= 1 || !strcmp(argv[1],"--help")  || !strcmp(argv[1],"-h")  ) {
        return fail_input();
    }
    int out = standard;
    char* filename = "";

    for (int i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "-q")) out            = quiet;
        else if (!strcmp(argv[i], "-t")) out            = timing;
        else if (!strcmp(argv[i], "-v")) out            = verbose;
        else if (filename[0] == '\0') filename       = argv[i];
    }

    if (out >= verbose) {
        printf("Input:");
        for (int i = 0; i < argc; i++) {
            printf(" %s", argv[i]);
        }
        printf("\n");
    }

    if (filename[0] == '\0') {
        if (out >= standard)
            printf("No filename found in input arguments.\n");
        return fail_input();
    }

    // Attempt to open program file first directly, then in the programs directory.
    const char* program_dir = "tests/programs/";
    FILE* read_ptr = fopen(filename, "r");
    if (read_ptr == NULL) {
        char retry_dir[strlen(program_dir) + strlen(filename) + 1];
        sprintf(retry_dir, "%s%s", program_dir, filename);
        read_ptr = fopen(retry_dir, "r");
        if (read_ptr == NULL) {
            if (out >= standard)
                printf("\x1b[31mError:\x1b[m Program file not found. Please double-check the name and read permission.\n");
            return fail_input();
        }
    }
    FILE* write_ptr = fopen("tokens.out", "w");
    if (write_ptr == NULL) {
        if (out >= verbose) {
            printf("\x1b[33mNote:\x1b[m Could not create \"tokens.out\" debugging file. Unnecessary, so proceeding.\n");
        }
        fclose(read_ptr);
    }

    struct state* state = malloc(sizeof(*state));
    state->out = out;
    state->output = write_ptr;

    unsigned long parse_before = -1;
    unsigned long parse_after  = -1;
    unsigned long eval_before  = -1;
    unsigned long eval_after   = -1;

    // Lexing
    state->ti = malloc(sizeof(*state->ti));
    state->ti->ts = malloc(sizeof(*state->ti->ts));
    state->ti->n = 0;
    unsigned long lex_before = microseconds();
    state->ti->n = lex(read_ptr, state);
    unsigned long lex_after = microseconds();
    if (out >= verbose)
        printf("\x1b[33mLex time: %luus. Read %lu tokens.\x1b[m\n", lex_after - lex_before, state->ti->n);
    if (read_ptr)   fclose(read_ptr);
    if (write_ptr)  fclose(write_ptr);

    if (state->ti->n) {
        /*if (out >= verbose) {
            for (int i = 0; i < state->ti->n; i++) {
                printf("Token #%d: \n", i);
                for (int c = 0; c < strlen(state->ti->ts[i].val); c++) {
                    printf("%c", state->ti->ts[i].val[c]);
                }
                printf("\n");
                printf("%s (%u)\n", state->ti->ts[i].val, state->ti->ts[i].type);
            }
        }*/
        FILE* token_file = fopen("tokens.out", "r");
        if (token_file == NULL) {
            if (out >= verbose)
                printf("\x1b[33mNote:\x1b[m Could not read \"tokens.out\" debugging file. Unnecessary, so proceeding.\n");
        }
        FILE* node_file = fopen("nodes.out", "w");
        if (node_file == NULL) {
            if (out >= verbose)
                printf("\x1b[33mNote:\x1b[m Could not create \"nodes.out\" debugging file. Unnecessary, so proceeding.\n");
            fclose(token_file);
            state->output = NULL;
        }
        else
            state->output = node_file;

        // Parsing
        parse_before = microseconds();
        state->tree = parse(token_file, state);
        parse_after = microseconds();
        if (state->tree == NULL) {
            if (out >= standard)
                printf("Could not parse file.\n");
            return 1;
        }
        if (out >= verbose) {
            printf("\x1b[33mParse time: %luus.\x1b[m\n", parse_after - parse_before);
            printf("Nodes: %d\n", state->tree->n);
            for (int i = 0; i < state->tree->n; i++) {
                printf("%3d: ", i);
                print_node(i, state);
                printf(". Par: %d. Fst: %d. Snd: %d.\n",
                    state->tree->ns[i].parent, state->tree->ns[i].first, state->tree->ns[i].second);
                for (int t = 0; t < state->tree->ns[i].token_count; t++) {
                    printf("%5d val: %s.\n", t, state->ti->ts[state->tree->ns[i].token_indices[t]].val);
                }
            }
        }
        /* for (int i = 0; i < state->nt->n; i++) {
            int indent = state->nt->nodes[i].print_indent;
            for (int ind = 0; ind < indent; ind++) {
                    if (ind == state->nt->nodes[state->nt->nodes[i].parent].print_indent) printf("| ");
                    else printf("  ");
            }

            printf("Node #%d. ", i);
            print_node(state->nt, i, out);
            printf(". Parent: %d. ", state->nt->nodes[i].parent);
            printf("1: %d. 2: %d\n", state->nt->nodes[i].first, state->nt->nodes[i].second);

            if (state->nt->nodes[i].token_count > 0) {
                for (int ind = 0; ind < indent; ind++) {
                    if (ind == state->nt->nodes[state->nt->nodes[i].parent].print_indent) printf("| ");
                    else printf("  ");
                }
                for (int j = 0; j < state->nt->nodes[i].token_count; j++) {
                    for (int c = 0;
                        c < strlen(state->ti->ts[state->nt->nodes[i].token_indices[j]].val); c++) {
                            putchar(state->ti->ts[state->nt->nodes[i].token_indices[j]].val[c]);
                        }
                        printf(" ");
                }
                printf("\n");
            }
        }
 */
        // Evaluating
        if (out >= verbose)
            printf("About to free token and node files.\n");
        if (token_file != NULL) fclose(token_file);
        if (node_file != NULL)  fclose(node_file);
        struct vtable_index* vtable = malloc(sizeof(*vtable));
        struct ftable_index* ftable = malloc(sizeof(*ftable));
        ftable->start_fun_node = 0;
        ftable->n = 0;
        vtable->n = 0;
        FILE* eval_output = fopen("eval.out", "w");
        if (eval_output == NULL && state->out >= verbose) {
            printf("\x1b[33mNote:\x1b[m Could not create \"eval.out\" debugging file. Unnecessary, so proceeding.\n");
        };
        state->output = eval_output;
        eval_before = microseconds();
        struct Val result = start_eval(vtable, ftable, state);
        eval_after = microseconds();
        if (out >= verbose)
            printf("\x1b[33mEval time: %luus.\x1b[m\n", eval_after - eval_before);
        print_val(stdout, result);
        printf("\n");
        if (eval_output) fclose(eval_output);
        if (out >= verbose) {
            for (int f = 0; f < ftable->n; f++) {
                printf("%3d %s %d", f, ftable->fs[f].id, ftable->fs[f].node_index);
                if (ftable->start_fun_node == ftable->fs[f].node_index) printf(" (main)\n");
                else printf("\n");
            }
        }
        if (out >= verbose)
            printf("Finishing up. Freeing allocated memory.\n");
        free_ftable(ftable);
        free_vtable(vtable);
        free_nodes(state->tree);
    }
    free_tokens(state->ti);
    free(state);
    unsigned long after = microseconds();
    if (out >= timing) {
        printf("Total time: %luus. Lex: %luus. Parse: %luus. Eval: %luus.\n",
            after - before, lex_after - lex_before, parse_after - parse_before,
            eval_after - eval_before);
    }
    return 0;
}
