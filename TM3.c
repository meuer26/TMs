#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAPE_LENGTH 1000
#define MAX_STEPS 100
#define DISPLAY_SIZE 25
#define NUM_SYMBOLS 15 // Symbols: 0, 1, 2 (2 for halt marker)
#define MAX_ITERATIONS 3 // Halt after 3 segments

typedef struct {
    int write_symbol;  // Symbol to write (0, 1, or 2)
    int move;          // -1 (left), 1 (right)
    int next_state;    // Next state
} Transition;

typedef struct {
    int state;            // Current state (0 to num_states-1, num_states for halt)
    int position;         // Tape position
    int halted;           // 1 if halted
    int step_count;       // Current step
    int iteration_count;  // Track processed segments
    int tape[TAPE_LENGTH]; // Tape
} Machine;

void init_tape(Machine *m) {
    memset(m->tape, 0, TAPE_LENGTH * sizeof(int));
    // Tape: ...0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, ... at 500–509
    m->tape[500] = 1;
    m->tape[501] = 0;
    m->tape[502] = 0;
    m->tape[503] = 1;
    m->tape[504] = 0;
    m->tape[505] = 0;
    m->tape[506] = 1;
    m->tape[507] = 0;
    m->tape[508] = 0;
    m->tape[509] = 2;
    // Debug: Verify tape initialization
    printf("Initial Tape (500–515): ");
    for (int i = 500; i <= 515; i++) {
        printf("%d ", m->tape[i]);
    }
    printf("\n");
    printf("Initial Tape: ");
    for (int i = m->position - DISPLAY_SIZE / 2; i <= m->position + DISPLAY_SIZE / 2; i++) {
        if (i >= 0 && i < TAPE_LENGTH) {
            printf(i == m->position ? "[%d]" : "%d", m->tape[i]);
        } else {
            printf(" ");
        }
        if (i < m->position + DISPLAY_SIZE / 2) printf(" ");
    }
    printf("\n");
}

void init_rules(Transition ***rules, int num_states) {
    *rules = malloc(num_states * sizeof(Transition *));
    if (!*rules) {
        printf("Error: Memory allocation failed for rules.\n");
        exit(1);
    }
    for (int i = 0; i < num_states; i++) {
        (*rules)[i] = malloc(NUM_SYMBOLS * sizeof(Transition));
        if (!(*rules)[i]) {
            printf("Error: Memory allocation failed for rules[%d].\n", i);
            exit(1);
        }
    }
    // Rules: Process [1,0,0] to [0,1,1] for three segments, halt on 2 in state 14
    for (int state = 0; state < num_states; state++) {
        for (int symbol = 0; symbol < NUM_SYMBOLS; symbol++) {
            // State 0: Start, find first segment
            if (state == 0 && symbol == 1) {
                (*rules)[state][symbol].write_symbol = 0;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 1;
            } else if (state == 0 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 0;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 0;
            } else if (state == 0 && symbol == 2) {
                (*rules)[state][symbol].write_symbol = 2;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 0;
            }
            // State 1: Process first 0 of first segment
            else if (state == 1 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 1;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 2;
            } else if (state == 1 && (symbol == 1 || symbol == 2)) {
                (*rules)[state][symbol].write_symbol = symbol;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 3;
            }
            // State 2: Process second 0 of first segment
            else if (state == 2 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 1;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 3;
            } else if (state == 2 && (symbol == 1 || symbol == 2)) {
                (*rules)[state][symbol].write_symbol = symbol;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 3;
            }
            // State 3: Navigate to second segment
            else if (state == 3 && symbol == 1) {
                (*rules)[state][symbol].write_symbol = 0;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 4;
            } else if (state == 3 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 0;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 3;
            } else if (state == 3 && symbol == 2) {
                (*rules)[state][symbol].write_symbol = 2;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 3;
            }
            // State 4: Process first 0 of second segment
            else if (state == 4 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 1;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 5;
            } else if (state == 4 && (symbol == 1 || symbol == 2)) {
                (*rules)[state][symbol].write_symbol = symbol;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 6;
            }
            // State 5: Process second 0 of second segment
            else if (state == 5 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 1;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 6;
            } else if (state == 5 && (symbol == 1 || symbol == 2)) {
                (*rules)[state][symbol].write_symbol = symbol;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 6;
            }
            // State 6: Navigate to third segment
            else if (state == 6 && symbol == 1) {
                (*rules)[state][symbol].write_symbol = 0;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 7;
            } else if (state == 6 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 0;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 6;
            } else if (state == 6 && symbol == 2) {
                (*rules)[state][symbol].write_symbol = 2;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 6;
            }
            // State 7: Process first 0 of third segment
            else if (state == 7 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 1;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 8;
            } else if (state == 7 && (symbol == 1 || symbol == 2)) {
                (*rules)[state][symbol].write_symbol = symbol;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 9;
            }
            // State 8: Process second 0 of third segment
            else if (state == 8 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 1;
                (*rules)[state][symbol].move = -1;
                (*rules)[state][symbol].next_state = 9;
            } else if (state == 8 && (symbol == 1 || symbol == 2)) {
                (*rules)[state][symbol].write_symbol = symbol;
                (*rules)[state][symbol].move = -1;
                (*rules)[state][symbol].next_state = 9;
            }
            // State 9: Verify third segment
            else if (state == 9 && symbol == 1) {
                (*rules)[state][symbol].write_symbol = 1;
                (*rules)[state][symbol].move = -1;
                (*rules)[state][symbol].next_state = 9;
            } else if (state == 9 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 0;
                (*rules)[state][symbol].move = -1;
                (*rules)[state][symbol].next_state = 10;
            } else if (state == 9 && symbol == 2) {
                (*rules)[state][symbol].write_symbol = 2;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 12;
            }
            // State 10: Verify second segment
            else if (state == 10 && symbol == 1) {
                (*rules)[state][symbol].write_symbol = 1;
                (*rules)[state][symbol].move = -1;
                (*rules)[state][symbol].next_state = 10;
            } else if (state == 10 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 0;
                (*rules)[state][symbol].move = -1;
                (*rules)[state][symbol].next_state = 11;
            } else if (state == 10 && symbol == 2) {
                (*rules)[state][symbol].write_symbol = 2;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 12;
            }
            // State 11: Verify first segment
            else if (state == 11 && symbol == 1) {
                (*rules)[state][symbol].write_symbol = 1;
                (*rules)[state][symbol].move = -1;
                (*rules)[state][symbol].next_state = 11;
            } else if (state == 11 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 0;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 12;
            } else if (state == 11 && symbol == 2) {
                (*rules)[state][symbol].write_symbol = 2;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 12;
            }
            // State 12: Navigate to halt marker
            else if (state == 12 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 0;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 13;
            } else if (state == 12 && symbol == 1) {
                (*rules)[state][symbol].write_symbol = 1;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 13;
            } else if (state == 12 && symbol == 2) {
                (*rules)[state][symbol].write_symbol = 2;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 14;
            }
            // State 13: Continue navigating to halt marker
            else if (state == 13 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 0;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 13;
            } else if (state == 13 && symbol == 1) {
                (*rules)[state][symbol].write_symbol = 1;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 13;
            } else if (state == 13 && symbol == 2) {
                (*rules)[state][symbol].write_symbol = 2;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 14;
            }
            // State 14: Check for halt
            else if (state == 14 && symbol == 0) {
                (*rules)[state][symbol].write_symbol = 0;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 12;
            } else if (state == 14 && symbol == 1) {
                (*rules)[state][symbol].write_symbol = 1;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 12;
            } else if (state == 14 && symbol == 2) {
                (*rules)[state][symbol].write_symbol = 2;
                (*rules)[state][symbol].move = 1;
                (*rules)[state][symbol].next_state = 12;
            }
            printf("State %d, Symbol %d: Write %d, Move %s, Next State %d\n",
                   state, symbol, (*rules)[state][symbol].write_symbol,
                   (*rules)[state][symbol].move == 1 ? "Right" : (*rules)[state][symbol].move == -1 ? "Left" : "Stay",
                   (*rules)[state][symbol].next_state);
        }
    }
}

void free_rules(Transition **rules, int num_states) {
    for (int i = 0; i < num_states; i++) {
        if (rules[i]) free(rules[i]);
    }
    free(rules);
}

void simulate(Machine *m, Transition **rules, int num_states) {
    while (m->step_count < MAX_STEPS && !m->halted) {
        m->step_count++;
        if (m->state < 0 || m->state > num_states) {
            printf("Error: Invalid state %d at step %d.\n", m->state, m->step_count);
            break;
        }
        int symbol = m->tape[m->position];
        if (symbol < 0 || symbol >= NUM_SYMBOLS) {
            printf("Error: Invalid symbol %d at step %d.\n", symbol, m->step_count);
            break;
        }
        Transition rule = rules[m->state][symbol];
        // Special case for state 14, symbol 2: check iteration count
        if (m->state == 14 && symbol == 2) {
            printf("Halt check: iteration_count=%d, MAX_ITERATIONS=%d\n", m->iteration_count, MAX_ITERATIONS);
            if (m->iteration_count >= MAX_ITERATIONS) {
                rule.write_symbol = 2;
                rule.move = 0;
                rule.next_state = num_states; // Halt state
            }
        }
        printf("\nStep %d: State=%d, Before Position=%d, Read=%d, Iteration Count=%d\n",
               m->step_count, m->state, m->position, symbol, m->iteration_count);
        
        // Display tape before action
        printf("Before Tape: ");
        int start = m->position - DISPLAY_SIZE / 2;
        int end = m->position + DISPLAY_SIZE / 2;
        for (int i = start; i <= end; i++) {
            if (i >= 0 && i < TAPE_LENGTH) {
                printf(i == m->position ? "[%d]" : "%d", m->tape[i]);
            } else {
                printf(" ");
            }
            if (i < end) printf(" ");
        }
        printf("\n");
        
        printf("Action: Write %d, Move %s, Next State %d\n",
               rule.write_symbol, rule.move == 1 ? "Right" : (rule.move == -1 ? "Left" : "Stay"), rule.next_state);
        
        m->tape[m->position] = rule.write_symbol;
        m->position += rule.move;
        if (m->position < 0 || m->position >= TAPE_LENGTH) {
            printf("Error: Tape position out of bounds at step %d.\n", m->step_count);
            m->halted = 1;
            break;
        }
        m->state = rule.next_state;
        
        // Increment iteration count after completing each segment
        if ((m->state == 3 && symbol == 1) || (m->state == 6 && symbol == 1) || (m->state == 9 && symbol == 0)) {
            m->iteration_count++;
            printf("Incrementing iteration_count to %d at state %d, symbol %d\n", m->iteration_count, m->state, symbol);
        }
        // Halt when entering state 15
        if (m->state == num_states) {
            m->halted = 1;
        }
        // Prevent indefinite looping in state 13
        if (m->state == 13 && m->position > 509 + 10) {
            printf("Error: Stuck in state 13, halting at step %d.\n", m->step_count);
            m->halted = 1;
            break;
        }
        
        printf("After Position: %d\n", m->position);
        
        printf("After Tape: ");
        start = m->position - DISPLAY_SIZE / 2;
        end = m->position + DISPLAY_SIZE / 2;
        for (int i = start; i <= end; i++) {
            if (i >= 0 && i < TAPE_LENGTH) {
                printf(i == m->position ? "[%d]" : "%d", m->tape[i]);
            } else {
                printf(" ");
            }
            if (i < end) printf(" ");
        }
        printf("\n");
        
        if (!m->halted) {
            printf("Press Enter to continue...\n");
            getchar();
        }
    }
    if (m->halted) {
        printf("Machine halted at step %d.\n", m->step_count);
    }
}

void print_final(Machine *m) {
    printf("\nFinal State: %d, Position=%d, Halted=%d, Halt Step=%d, Iteration Count=%d\n",
           m->state, m->position, m->halted, m->step_count, m->iteration_count);
    printf("Final Tape: ");
    int start = m->position - DISPLAY_SIZE / 2;
    int end = m->position + DISPLAY_SIZE / 2;
    for (int i = start; i <= end; i++) {
        if (i >= 0 && i < TAPE_LENGTH) {
            printf(i == m->position ? "[%d]" : "%d", m->tape[i]);
        } else {
            printf(" ");
        }
        if (i < end) printf(" ");
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    int num_states = 15; // 15 states (0-14), plus halt state (15)
    if (argc > 1) {
        num_states = atoi(argv[1]);
        if (num_states < 1 || num_states > 100) {
            printf("Error: Number of states must be between 1 and 100.\n");
            return 1;
        }
    }
    printf("Starting Turing Machine simulation with %d states (plus halt state %d)...\n", num_states, num_states);
    Machine m = {0, 500, 0, 0, 0, {0}}; // Initialize with state 0, position 500
    Transition **rules;
    init_tape(&m);
    init_rules(&rules, num_states);
    simulate(&m, rules, num_states);
    print_final(&m);
    free_rules(rules, num_states);
    return 0;
}