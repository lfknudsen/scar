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

void print_node(struct tree* node_tree, int n_index) {
    fprint_node(node_tree, n_index, stdout);
}

void fprint_node(struct tree* node_tree, int n_index, FILE* output) {
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

void free_tokens(struct token_index *ti) {
    for (int i = 0; i < ti->n; i++) {
        free(ti->ts[i].val);
    }
    free(ti->ts);
    free(ti);
}

int fail_input() {
    printf("Scar Compiler usage:\n./scar <filename>\n");
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc <= 1 || strcmp(argv[1],"--help") == 0 || strcmp(argv[1],"-h") == 0 || argc > 3 ) {
        return fail_input();
    }
    int quiet = 1-VERBOSE;
    if (argc == 3 && strcmp(argv[2],"-t") == 0) {
        quiet = 1;
    }
    if (VERBOSE) printf("Input: %s\n", argv[1]);
    FILE *read_ptr;
    read_ptr = fopen(argv[1], "r");
    if (read_ptr == NULL) { if (VERBOSE) printf("File not found.\n"); return fail_input(); }
    FILE *write_ptr;
    write_ptr = fopen("tokens.out", "w");
    if (write_ptr == NULL) {
        if (VERBOSE) printf("Could not create/write to file. Check permissions.\n");
        fclose(read_ptr);
        return 1;
    }

    struct token_index *ti = malloc(sizeof(*ti));
    ti->ts = malloc(sizeof(*ti->ts));
    ti->n = 0;
    ti->n = lex(read_ptr, write_ptr, ti);
    if (!quiet) printf("Read %lu tokens.\n", ti->n);
    fclose(read_ptr);
    fclose(write_ptr);

    if (ti->n) {
        if (!quiet) {
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
        if (token_file == NULL) { printf("Token file not found.\n"); return 1; }
        FILE* node_file = fopen("nodes.out", "w");
        if (node_file == NULL) {
            printf("Could not create node file.\n");
            fclose(token_file);
            return 1;
        }
        struct tree* node_tree = parse(token_file, node_file, ti);
        if (node_tree == NULL) { printf("Could not parse file.\n"); return 1; }
        if (!quiet) {
            printf("Nodes: %d\n", node_tree->n);
            for (int i = 0; i < node_tree->n; i++) {
                printf("%3d: ", i);
                print_node(node_tree, i);
                printf(". %s. Par: %d. Fst: %d. Snd: %d.\n", ti->ts[node_tree->nodes[i].token_indices[0]].val,
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
            print_node(node_tree, i);
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
 */		fclose(token_file);
        fclose(node_file);
        struct ivtable_index* vtable = malloc(sizeof(vtable));
        struct ftable_index* ftable = malloc(sizeof(ftable));
        ftable->start_fun_node = 0;
        ftable->n = 0;
        FILE* eval_output = fopen("eval.out", "w");
        int result = start_eval(node_tree, 0, ti, eval_output, vtable, ftable);
        printf("%d\n", result);
        fclose(eval_output);
        if (!quiet) {
            for (int f = 0; f < ftable->n; f++) {
                printf("%3d %s %d", f, ftable->fs[f].id, ftable->fs[f].node_index);
                if (ftable->start_fun_node == ftable->fs[f].node_index) printf(" (main)\n");
                else printf("\n");
            }
        }
    }
    free_tokens(ti);
    return 0;
}
