// Not a universal TM. We need at least 10 states with two symbols.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAPE_LENGTH 1000
#define MAX_STEPS 500
#define DISPLAY_SIZE 25
#define NUM_SYMBOLS 3 // Symbols: 0, 1, 2 (2 for halting)

typedef struct {
    int write_symbol;  // Symbol to write (0, 1, or 2)
    int move;          // Tape movement: -1 (left), 1 (right)
    int next_state;    // Next state
} Rule;

typedef struct {
    int current_state;    // Current state (0 to num_states-1, num_states for halt)
    int tape_position;    // Current position on tape
    int halted;           // 1 if halted
    int halt_step;        // Step at which machine halted
    int tape[TAPE_LENGTH]; // Input tape
} TuringMachine;

void initialize_tape(TuringMachine *tm) {
    memset(tm->tape, 0, TAPE_LENGTH * sizeof(int));
    // Set up tape: ...0, 1, 0, 1, 0, 1, 0, 2, ...
    tm->tape[500] = 1;
    tm->tape[501] = 0;
    tm->tape[502] = 1;
    tm->tape[503] = 0;
    tm->tape[504] = 1;
    tm->tape[505] = 0;
    tm->tape[506] = 2; // Trigger halt after three loops
    printf("Initial Tape: ");
    for (int i = tm->tape_position - DISPLAY_SIZE / 2; i <= tm->tape_position + DISPLAY_SIZE / 2; i++) {
        if (i >= 0 && i < TAPE_LENGTH) {
            if (i == tm->tape_position) {
                printf("[%d]", tm->tape[i]);
            } else {
                printf("%d", tm->tape[i]);
            }
        } else {
            printf(" ");
        }
        if (i < tm->tape_position + DISPLAY_SIZE / 2) printf(" ");
    }
    printf("\n");
}

void setup_rules(Rule ***rules, int num_states) {
    *rules = malloc(num_states * sizeof(Rule *));
    for (int i = 0; i < num_states; i++) {
        (*rules)[i] = malloc(NUM_SYMBOLS * sizeof(Rule));
    }
    // Rules: Loop by flipping [1,0] to [0,1] with state cycle 0->1->0, moving left/right, halt on 2 in state 1
    for (int state = 0; state < num_states; state++) {
        for (int symbol = 0; symbol < NUM_SYMBOLS; symbol++) {
            if (state == 1 && symbol == 2) {
                // Clear halting rule: In state 1, reading a 2 halts (go to state num_states)
                (*rules)[state][symbol].write_symbol = 2;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = num_states;
            } else if (state == 0 && symbol == 1) {
                // Loop: Read 1 in state 0, write 0, move right, go to state 1
                (*rules)[state][symbol].write_symbol = 0;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 1;
            } else if (state == 1 && symbol == 0) {
                // Loop: Read 0 in state 1, write 1, move left, go to state 0
                (*rules)[state][symbol].write_symbol = 1;
                (*rules)[state][symbol].move = -1;
                (*rules)[state][symbol].next_state = 0;
            } else if (state == 0 && symbol == 0) {
                // Skip 0s in state 0, move right
                (*rules)[state][symbol].write_symbol = 0;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 0;
            } else if (state == 1 && symbol == 1) {
                // Continue processing 1s in state 1, move right
                (*rules)[state][symbol].write_symbol = 1;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 1;
            } else {
                // Maintain state for unused cases (e.g., state 0, symbol 2)
                (*rules)[state][symbol].write_symbol = symbol;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = state;
            }
            printf("State %d, Symbol %d: Write %d, Move %s, Next State %d\n",
                   state, symbol, (*rules)[state][symbol].write_symbol,
                   (*rules)[state][symbol].move == 1 ? "Right" : "Left",
                   (*rules)[state][symbol].next_state);
        }
    }
}

void free_rules(Rule **rules, int num_states) {
    for (int i = 0; i < num_states; i++) {
        free(rules[i]);
    }
    free(rules);
}

void simulate(TuringMachine *tm, Rule **rules, int num_states) {
    for (int step = 1; step <= MAX_STEPS; step++) {
        if (tm->halted) {
            printf("Machine halted at step %d.\n", tm->halt_step);
            break;
        }
        int symbol = tm->tape[tm->tape_position];
        Rule rule = rules[tm->current_state][symbol];
        printf("\nStep %d: State=%d, Position=%d, Read=%d\n",
               step, tm->current_state, tm->tape_position, symbol);
        
        // Display tape before action
        printf("Before Tape: ");
        int start = tm->tape_position - DISPLAY_SIZE / 2;
        int end = tm->tape_position + DISPLAY_SIZE / 2;
        for (int i = start; i <= end; i++) {
            if (i >= 0 && i < TAPE_LENGTH) {
                if (i == tm->tape_position) {
                    printf("[%d]", tm->tape[i]);
                } else {
                    printf("%d", tm->tape[i]);
                }
            } else {
                printf(" ");
            }
            if (i < end) printf(" ");
        }
        printf("\n");
        
        printf("Action: Write %d, Move %s, Next State %d\n",
               rule.write_symbol, rule.move == 1 ? "Right" : "Left", rule.next_state);
        tm->tape[tm->tape_position] = rule.write_symbol;
        tm->tape_position += rule.move;
        if (tm->tape_position < 0 || tm->tape_position >= TAPE_LENGTH) {
            printf("Error: Tape position out of bounds at step %d.\n", step);
            break;
        }
        tm->current_state = rule.next_state;
        tm->halt_step = step;
        if (tm->current_state == num_states) {
            tm->halted = 1;
        }
        
        // Display new position after action
        printf("After Position: %d\n", tm->tape_position);
        
        // Display tape after action
        printf("After Tape: ");
        start = tm->tape_position - DISPLAY_SIZE / 2;
        end = tm->tape_position + DISPLAY_SIZE / 2;
        for (int i = start; i <= end; i++) {
            if (i >= 0 && i < TAPE_LENGTH) {
                if (i == tm->tape_position) {
                    printf("[%d]", tm->tape[i]);
                } else {
                    printf("%d", tm->tape[i]);
                }
            } else {
                printf(" ");
            }
            if (i < end) printf(" ");
        }
        printf("\n");
        
        if (!tm->halted) {
            printf("Press Enter to continue...\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF); // Clear input buffer
        }
    }
}

void print_final_state(TuringMachine *tm) {
    printf("\nFinal State: %d, Position: %d, Halted: %d, Halt Step: %d\n",
           tm->current_state, tm->tape_position, tm->halted, tm->halt_step);
    printf("Final Tape: ");
    int start = tm->tape_position - DISPLAY_SIZE / 2;
    int end = tm->tape_position + DISPLAY_SIZE / 2;
    for (int i = start; i <= end; i++) {
        if (i >= 0 && i < TAPE_LENGTH) {
            if (i == tm->tape_position) {
                printf("[%d]", tm->tape[i]);
            } else {
                printf("%d", tm->tape[i]);
            }
        } else {
            printf(" ");
        }
        if (i < end) printf(" ");
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    int num_states = 2; // Default to 2 states (0, 1, with 2 as halt)
    if (argc > 1) {
        num_states = atoi(argv[1]);
        if (num_states < 1 || num_states > 100) {
            printf("Error: Number of states must be between 1 and 100.\n");
            return 1;
        }
    }
    printf("Starting Turing Machine simulation with %d states...\n", num_states);
    TuringMachine tm = {0, 500, 0, 0, {0}};
    Rule **rules;
    initialize_tape(&tm);
    setup_rules(&rules, num_states);
    simulate(&tm, rules, num_states);
    print_final_state(&tm);
    free_rules(rules, num_states);
    return 0;
}