#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include "scar.h"
#include "lexer.h"
#include "parser.h"
#include "eval.h"

// For now, grammar is as follows:
// FunDec 		-> Type Id ( FunArgs ) = { BodyStats }
// BodyStats 	-> Stats ; BodyStats
// BodyStats 	-> return Exp ;
// BodyStats	-> return ;
// Type 		-> int
// Type 		-> float
// FunArgs 		-> Type Id, FunArgs
// FunArgs 		-> Type Id
// FunArgs 		->
// Stats 		-> Stat ; Stats
// Stats 		-> Stat ;
// Stat 		-> Type Id = Val
// Stat			-> return Exp ;
// Stat			-> return ;
// Val			-> num
// Val			-> id
// Exp 			-> Exp Binop Exp
// Exp			-> Val
// Binop		-> +
// Binop		-> -
// Binop		-> *
// Binop		-> /

void print_node(struct tree* node_tree, int n_index, int out) {
    fprint_node(node_tree, n_index, stdout, out);
}

void fprint_node(struct tree* node_tree, int n_index, FILE* output, int out) {
    if (output == NULL) {
        output = stdout;
    }
    switch (node_tree->nodes[n_index].nodetype) {
        case n_stat:
            switch (node_tree->nodes[n_index].specific_type) {
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
            }
        case n_expr:
            switch (node_tree->nodes[n_index].specific_type) {
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
            }
        case n_prog:
            fprintf(output, "Top-level program");
            return;
        default:
            return;
    }
    return;
}

void free_tokens(struct token_index* ti) {
    for (int i = 0; i < ti->n; i++) {
        free(ti->ts[i].val);
    }
    free(ti->ts);
    free(ti);
}

void free_nodes(struct tree* nt) {
    for (int i = 0; i < nt->n; i++) {
        free(nt->nodes[i].token_indices);
    }
    free(nt->nodes);
    free(nt);
}

void free_ftable(struct ftable_index* ft) {
    free(ft->fs);
    free(ft);
}

void free_vtable(struct vtable_index* vt) {
    free(vt->vs);
    free(vt);
}

void free_ivtable(struct ivtable_index* vt) {
    //free(vt->vs);
    free(vt);
}

int fail_input() {
    printf("Scar Compiler usage:\n./scar [option] <filename>\n");
    printf("Options:\n  -v    Print in-depth log information.\n");
    printf("  -q    Hide all print output, including errors, excluding result.\n");
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc <= 1 || strcmp(argv[1],"--help") == 0 || strcmp(argv[1],"-h") == 0 ) {
        return fail_input();
    }
    int out = standard;
    if (argc >= 3) {
        if (strcmp(argv[1],"-q") == 0) {
            out = quiet;
        } else if (strcmp(argv[1],"-v") == 0) {
            printf("Verbose output activated.\n");
            out = verbose;
        }
    }
    if (out == verbose) {
        printf("Input:");
        for (int i = 0; i < argc; i++) {
            printf(" %s", argv[i]);
        }
        printf("\n");
    }
    const char* program_dir = "tests/programs/";
    char* filename;
    if (argc == 2) filename = argv[1];
    else if (argc >= 3) filename = argv[2];
    FILE* read_ptr = fopen(filename, "r");
    if (read_ptr == NULL) {
        //printf("About to retry.\n");
        char* retry_dir = malloc(strlen(program_dir) + strlen(filename));
        sprintf(retry_dir, "%s%s", program_dir, filename);
        read_ptr = fopen(retry_dir, "r");
        free(retry_dir);
        if (read_ptr == NULL) {
            if (out >= standard) printf("File not found.\n");
            return fail_input();
        }
    }
    FILE *write_ptr;
    write_ptr = fopen("tokens.out", "w");
    if (write_ptr == NULL) {
        if (out >= standard) printf("Could not create/write to file. Check permissions.\n");
        fclose(read_ptr);
        return 1;
    }

    // Lexing
    struct token_index *ti = malloc(sizeof(*ti));
    ti->ts = malloc(sizeof(*ti->ts));
    ti->n = 0;
    ti->n = lex(read_ptr, write_ptr, ti);
    if (out >= verbose) printf("Read %lu tokens.\n", ti->n);
    fclose(read_ptr);
    fclose(write_ptr);

    if (ti->n) {
        if (out >= verbose) {
            for (int i = 0; i < ti->n; i++) {
                printf("Token #%d: \n", i);
                for (int c = 0; c < strlen(ti->ts[i].val); c++) {
                    printf("%c", ti->ts[i].val[c]);
                }
                printf("\n");
                printf("%s (%u)\n", ti->ts[i].val, ti->ts[i].type);
            }
        }
        FILE* token_file = fopen("tokens.out", "r");
        if (token_file == NULL) {
            if (out >= standard) printf("Token file not found.\n");
            return 1;
        }
        FILE* node_file = fopen("nodes.out", "w");
        if (node_file == NULL) {
            if (out >= standard) printf("Could not create node file.\n");
            fclose(token_file);
            return 1;
        }
        // Parsing
        struct tree* node_tree = parse(token_file, node_file, ti, out);
        if (node_tree == NULL) { if (out >= standard) printf("Could not parse file.\n"); return 1; }
        if (out >= verbose) {
            printf("Nodes: %d\n", node_tree->n);
            for (int i = 0; i < node_tree->n; i++) {
                printf("%3d: ", i);
                print_node(node_tree, i, out);
                printf(". Par: %d. Fst: %d. Snd: %d.\n", 
                node_tree->nodes[i].parent, node_tree->nodes[i].first, node_tree->nodes[i].second);
                for (int t = 0; t < node_tree->nodes[i].token_count; t++) {
                    printf("%5d val: %s.\n", t, ti->ts[node_tree->nodes[i].token_indices[t]].val);
                }
            }
        }
        /* for (int i = 0; i < node_tree->n; i++) {
            int indent = node_tree->nodes[i].print_indent;
            for (int ind = 0; ind < indent; ind++) {
                    if (ind == node_tree->nodes[node_tree->nodes[i].parent].print_indent) printf("| ");
                    else printf("  ");
            }

            printf("Node #%d. ", i);
            print_node(node_tree, i, out);
            printf(". Parent: %d. ", node_tree->nodes[i].parent);
            printf("1: %d. 2: %d\n", node_tree->nodes[i].first, node_tree->nodes[i].second);

            if (node_tree->nodes[i].token_count > 0) {
                for (int ind = 0; ind < indent; ind++) {
                    if (ind == node_tree->nodes[node_tree->nodes[i].parent].print_indent) printf("| ");
                    else printf("  ");
                }
                for (int j = 0; j < node_tree->nodes[i].token_count; j++) {
                    for (int c = 0;
                        c < strlen(ti->ts[node_tree->nodes[i].token_indices[j]].val); c++) {
                            putchar(ti->ts[node_tree->nodes[i].token_indices[j]].val[c]);
                        }
                        printf(" ");
                }
                printf("\n");
            }
        }
 */
        // Evaluating
        fclose(token_file);
        fclose(node_file);
        struct ivtable_index* vtable = malloc(sizeof(*vtable));
        struct ftable_index* ftable = malloc(sizeof(*ftable));
        ftable->start_fun_node = 0;
        ftable->n = 0;
        ftable->fs = malloc(sizeof(ftable->fs));
        vtable->vs = malloc(sizeof(vtable->vs));
        vtable->n = 0;
        FILE* eval_output = fopen("eval.out", "w");
        int result = start_eval(node_tree, 0, ti, eval_output, vtable, ftable, out);
        if (out >= verbose) printf("Returned from final evaluation.\n");
        printf("%d\n", result);
        fclose(eval_output);
        if (out >= verbose) {
            for (int f = 0; f < ftable->n; f++) {
                printf("%3d %s %d", f, ftable->fs[f].id, ftable->fs[f].node_index);
                if (ftable->start_fun_node == ftable->fs[f].node_index) printf(" (main)\n");
                else printf("\n");
            }
        }
        if (out >= verbose) printf("Finishing up. Freeing allocated memory.\n");
        free_ftable(ftable);
        free_ivtable(vtable);
        free_nodes(node_tree);
    }
    free_tokens(ti);
    return 0;
}
