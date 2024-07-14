#include <stdio.h>

struct state_info {
    int self = -1;
    int out1 = -1;
    int out2 = -1;
    int out3 = -1;
};

int main(int argc, char* argv[]) {
    char* filename;
    if (argc == 1) {
        filename = "parser.c";
    }
    else if (argc == 2) {
        filename = argv[1];
    }
    printf("Input: %s\n", filename);

    FILE* read_ptr;
    read_ptr = fopen(filename, "r");

    if (read_ptr == NULL) { printf("File not found.\n"); return 1; }
    FILE* write_ptr;
    write_ptr = fopen("states.out", "w");
    if (write_ptr == NULL) {
        printf("File could not be created.\n");
        fclose(read_ptr);
        return 1;
    }

    int correct_letters_ifstate = 0;
    int correct_letters_parses = 0;
    int current_state = -1;
    struct state* states;
    int number_of_states = 0;
    int number_of_parse_s_encountered;
    char c = fgetc(read_ptr);
    while (c != EOF) {
        if (c == ' ') continue;
        else if (correct_letters_ifstate == 0 && c == 'i') correct_letters_ifstate ++;
        else if (correct_letters_ifstate == 1 && c == 'f') correct_letters_ifstate ++;
        else if (correct_letters_ifstate == 2 && c == '(') correct_letters_ifstate ++;
        else if (correct_letters_ifstate == 3 && c == 's') correct_letters_ifstate ++;
        else if (correct_letters_ifstate == 4 && c == 't') correct_letters_ifstate ++;
        else if (correct_letters_ifstate == 5 && c == 'a') correct_letters_ifstate ++;
        else if (correct_letters_ifstate == 6 && c == 't') correct_letters_ifstate ++;
        else if (correct_letters_ifstate == 7 && c == 'e') correct_letters_ifstate ++;
        else if (correct_letters_ifstate == 8 && c == '=') correct_letters_ifstate ++;
        else if (correct_letters_ifstate == 9 && c == '=') correct_letters_ifstate ++;
        else if (correct_letters_ifstate == 10 && c >= 48 && c <= 57) {
            current_state *= 10;
            current_state += int(c);
        }
        else if (correct_letters_ifstate == 10 && c == ')') {
            number_of_states ++;
            struct state_info new_state = malloc(sizeof(struct state_info));
            states = realloc(sizeof(states) * number_of_states);
            current_state = number_of_states - 1;
            states[current_state] = new_state;
            correct_letters_ifstate = 0;
        }
        else if (current_state != -1 && correct_letters_parses == 0 && c == 'p') correct_letters_parses ++;
        else if (current_state != -1 && correct_letters_parses == 1 && c == 'a') correct_letters_parses ++;
        else if (current_state != -1 && correct_letters_parses == 2 && c == 'r') correct_letters_parses ++;
        else if (current_state != -1 && correct_letters_parses == 3 && c == 's') correct_letters_parses ++;
        else if (current_state != -1 && correct_letters_parses == 4 && c == 'e') correct_letters_parses ++;
        else if (current_state != -1 && correct_letters_parses == 5 && c == '_') correct_letters_parses ++;
        else if (current_state != -1 && correct_letters_parses == 6 && c == 's') correct_letters_parses ++;
        else if (current_state != -1 && correct_letters_parses == 7 && c == '(') correct_letters_parses ++;
        else if (current_state != -1 && correct_letters_parses == 8 && c >= 48 && c <= 57) {
            if (number_of_parse_s_encountered == 0) {
                if (states[current_state].out1 == -1) states[current_state].out1 = 0; 
                states[current_state].out1 *= 10;
                states[current_state].out1 += int(c);
            }
            else if (number_of_parse_s_encountered == 1) {
                if (states[current_state].out2 == -1) states[current_state].out2 = 0; 
                states[current_state].out2 *= 10;
                states[current_state].out2 += int(c);
            }
            else if (number_of_parse_s_encountered == 2) {
                if (states[current_state].out3 == -1) states[current_state].out3 = 0; 
                states[current_state].out3 *= 10;
                states[current_state].out3 += int(c);
            }
        }
        else if (current_state != -1 && correct_letters == 8 && c == ',') number_of_parse_s_encountered ++;

        c = fgetc(read_ptr);
    }
}