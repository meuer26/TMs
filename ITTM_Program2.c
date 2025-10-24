#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Configuration: ITTM simulation for teaching, 20 three-state machines
#define NUM_MACHINES 20
#define NUM_STATES 3
#define NUM_SYMBOLS 2
#define MAX_STEPS 500
#define WINDOW_SIZE 12
#define TAPE_LENGTH 1000

// Structure for each Turing machine
typedef struct {
    uint8_t current_state;   // Current state (0, 1, or 2 for halt)
    uint32_t tape_position;  // Position on input tape
    uint8_t halted;          // 1 if halted or looped
    uint32_t halt_step;      // Step at which machine halted or looped
    uint8_t tape[TAPE_LENGTH]; // Individual input tape
} TuringMachine;

// Structure for transition rules
typedef struct {
    uint8_t write_symbol;    // Symbol to write (0 or 1)
    uint8_t next_state;      // Next state (0, 1, or 2)
} Rule;

// Global state
TuringMachine tms[NUM_MACHINES];              // Tape 2: machine states
uint8_t output_tape[NUM_MACHINES][WINDOW_SIZE]; // Tape 3: simulation window
uint8_t halt_set[NUM_MACHINES / 8 + 1];       // Tape 4: halting set bitmap
Rule rule_table[NUM_MACHINES][NUM_STATES][NUM_SYMBOLS];

// Initialize each machine's input tape to all 0s, except Machine 9
void initialize_tapes() {
    for (int i = 0; i < NUM_MACHINES; i++) {
        memset(tms[i].tape, 0, TAPE_LENGTH);
        if (i == 9) tms[i].tape[1] = 1; // Add a 1 for Machine 9 to trigger state transition
    }
    printf("Tape 1: Blank input = %.50s...\n", tms[0].tape);
    printf("Tape 1 (Machine 9) = %.50s...\n", tms[9].tape);
}

// Set up rules for 20 machines (10 looping, 10 halting)
void setup_rules() {
    // Define balanced rules
    Rule rules[NUM_MACHINES][NUM_STATES][NUM_SYMBOLS] = {
        // Machine 0: Loop (stay in state 0)
        {{{0,0},{0,0}}, {{0,0},{0,0}}, {{0,0},{0,0}}},
        // Machine 1: Halt after two steps
        {{{1,1},{1,1}}, {{1,2},{1,2}}, {{0,0},{0,0}}},
        // Machine 2: Loop (cycle 0↔1)
        {{{0,1},{1,0}}, {{1,0},{0,1}}, {{0,0},{0,0}}},
        // Machine 3: Loop (write 1, stay in 1)
        {{{1,1},{1,1}}, {{1,1},{1,1}}, {{0,0},{0,0}}},
        // Machine 4: Halt after two steps
        {{{1,1},{1,1}}, {{0,2},{0,2}}, {{0,0},{0,0}}},
        // Machine 5: Halt immediately
        {{{1,2},{1,2}}, {{0,0},{0,0}}, {{0,0},{0,0}}},
        // Machine 6: Loop (write 0, stay in 1)
        {{{0,1},{0,1}}, {{0,1},{0,1}}, {{0,0},{0,0}}},
        // Machine 7: Halt after two steps
        {{{1,1},{1,1}}, {{1,2},{1,2}}, {{0,0},{0,0}}},
        // Machine 8: Halt immediately
        {{{0,2},{0,2}}, {{0,0},{0,0}}, {{0,0},{0,0}}},
        // Machine 9: Halt after three steps
        {{{0,0},{1,1}}, {{0,2},{0,2}}, {{0,0},{0,0}}},
        // Machine 10: Loop (write 1, cycle 0↔1)
        {{{1,1},{1,0}}, {{1,0},{1,1}}, {{0,0},{0,0}}},
        // Machine 11: Halt immediately
        {{{0,2},{0,2}}, {{0,0},{0,0}}, {{0,0},{0,0}}},
        // Machine 12: Halt after two steps
        {{{0,1},{0,1}}, {{0,2},{0,2}}, {{0,0},{0,0}}},
        // Machine 13: Halt immediately
        {{{1,2},{1,2}}, {{0,0},{0,0}}, {{0,0},{0,0}}},
        // Machine 14: Loop (stay in state 0)
        {{{0,0},{0,0}}, {{0,0},{0,0}}, {{0,0},{0,0}}},
        // Machine 15: Loop (stay in state 0)
        {{{0,0},{0,0}}, {{0,0},{0,0}}, {{0,0},{0,0}}},
        // Machine 16: Loop (write 0, cycle 0↔1)
        {{{0,1},{0,0}}, {{0,0},{0,1}}, {{0,0},{0,0}}},
        // Machine 17: Halt immediately
        {{{0,2},{0,2}}, {{0,0},{0,0}}, {{0,0},{0,0}}},
        // Machine 18: Loop (write 0, cycle 0↔1)
        {{{0,1},{0,0}}, {{0,0},{0,1}}, {{0,0},{0,0}}},
        // Machine 19: Loop (write 0, cycle 0↔1)
        {{{0,1},{0,0}}, {{1,0},{1,1}}, {{0,0},{0,0}}}
    };
    // Initialize machines and copy rules
    for (int i = 0; i < NUM_MACHINES; i++) {
        tms[i].current_state = 0;
        tms[i].tape_position = 0;
        tms[i].halted = 0;
        tms[i].halt_step = 0;
        for (int j = 0; j < WINDOW_SIZE; j++) output_tape[i][j] = 0;
        for (int s = 0; s < NUM_STATES; s++) {
            for (int sym = 0; sym < NUM_SYMBOLS; sym++) {
                rule_table[i][s][sym] = rules[i][s][sym];
            }
        }
        printf("Machine %d: Rules=[0->%d,%d] [1->%d,%d] [0->%d,%d]\n",
               i, rule_table[i][0][0].write_symbol, rule_table[i][0][0].next_state,
               rule_table[i][0][1].write_symbol, rule_table[i][0][1].next_state,
               rule_table[i][1][0].write_symbol, rule_table[i][1][0].next_state);
    }
    memset(halt_set, 0, sizeof(halt_set));
    printf("Rule generation completed for all machines.\n");
}

// Check for loops in Tape 3 and state periodicity
int detect_loop(int machine_idx, uint32_t step) {
    if (step < 100) return 0; // Threshold for loop detection
    // Check Tape 3 periodicity
    for (int period = 1; period <= WINDOW_SIZE / 2; period++) {
        int is_loop = 1;
        for (int i = 0; i < period; i++) {
            int idx1 = (step - i - 1) % WINDOW_SIZE;
            int idx2 = (step - i - 1 - period) % WINDOW_SIZE;
            if (output_tape[machine_idx][idx1] != output_tape[machine_idx][idx2]) {
                is_loop = 0;
                break;
            }
        }
        if (is_loop) return 1;
    }
    // Check state periodicity
    static uint8_t past_states[NUM_MACHINES][WINDOW_SIZE];
    static uint32_t state_steps[NUM_MACHINES][WINDOW_SIZE];
    if (step < WINDOW_SIZE) {
        past_states[machine_idx][step % WINDOW_SIZE] = tms[machine_idx].current_state;
        state_steps[machine_idx][step % WINDOW_SIZE] = step;
    } else {
        for (int period = 1; period <= WINDOW_SIZE / 2; period++) {
            int is_loop = 1;
            for (int i = 0; i < period; i++) {
                int idx1 = (step - i - 1) % WINDOW_SIZE;
                int idx2 = (step - i - 1 - period) % WINDOW_SIZE;
                if (past_states[machine_idx][idx1] != past_states[machine_idx][idx2]) {
                    is_loop = 0;
                    break;
                }
            }
            if (is_loop) return 1;
        }
    }
    return 0;
}

// Simulate all machines in dovetailed fashion with pause after each step
void simulate() {
    for (uint32_t step = 1; step <= MAX_STEPS; step++) {
        int all_halted = 1;
        printf("Step %u:\n", step);
        // Perform one step for each non-halted machine and print steps
        for (int m = 0; m < NUM_MACHINES; m++) {
            if (tms[m].halted) continue;
            all_halted = 0;
            uint8_t symbol = tms[m].tape[tms[m].tape_position % TAPE_LENGTH];
            uint8_t write = rule_table[m][tms[m].current_state][symbol].write_symbol;
            uint8_t next = rule_table[m][tms[m].current_state][symbol].next_state;
            printf("Machine %d: Step %u, Read %d, Write %d, Next State %d\n",
                   m, step, symbol, write, next);
            if (tms[m].halt_step < WINDOW_SIZE) {
                output_tape[m][tms[m].halt_step] = write;
            }
            tms[m].tape[tms[m].tape_position % TAPE_LENGTH] = write;
            tms[m].current_state = next;
            tms[m].tape_position++;
            tms[m].halt_step = step;
            if (next == 2 || (step >= 100 && detect_loop(m, step))) {
                tms[m].halted = 1;
                if (next == 2) {
                    halt_set[m / 8] |= (1 << (m % 8));
                }
            }
        }
        // Print Tape 2 and Tape 3 combined for each machine with tab alignment and active symbol highlighted
        printf("Machine States and Simulation Window:\n");
        for (int i = 0; i < NUM_MACHINES; i++) {
            printf("Machine %d:\tState=%d,\tPos=%u,\tDone=%d,\tHaltStep=%u,\tTape3=[",
                   i, tms[i].current_state, tms[i].tape_position, tms[i].halted, tms[i].halt_step);
            for (int j = 0; j < WINDOW_SIZE; j++) {
                if (!tms[i].halted && tms[i].halt_step == step && j == step - 1 && j < WINDOW_SIZE) {
                    printf("[%d]", output_tape[i][j]); // Highlight active symbol
                } else {
                    printf("%d", output_tape[i][j]);
                }
                if (j < WINDOW_SIZE - 1) printf(",");
            }
            printf("]\n");
        }
        // Pause and wait for key press (Enter)
        if (!all_halted) {
            printf("Press Enter to continue...\n");
            getchar(); // Wait for Enter key
        }
        if (all_halted) break; // Stop if all machines are done
    }
}

// Print Tape 2 and Tape 3 combined with tab alignment
void print_tapes() {
    printf("Final Machine States and Simulation Window:\n");
    for (int i = 0; i < NUM_MACHINES; i++) {
        printf("Machine %d:\tState=%d,\tPos=%u,\tDone=%d,\tHaltStep=%u,\tTape3=[",
               i, tms[i].current_state, tms[i].tape_position, tms[i].halted, tms[i].halt_step);
        for (int j = 0; j < WINDOW_SIZE; j++) {
            printf("%d", output_tape[i][j]);
            if (j < WINDOW_SIZE - 1) printf(",");
        }
        printf("]\n");
    }
}

// Print Tape 4: halting set
void print_halt_set() {
    int halts = 0;
    printf("Tape 4 (1=halted):\n");
    for (int i = 0; i < NUM_MACHINES; i++) {
        int bit = (halt_set[i / 8] >> (i % 8)) & 1;
        printf("%d", bit);
        halts += bit;
        if (i % 8 == 7) printf(" ");
    }
    printf("\nHalted: %d/%d\n", halts, NUM_MACHINES);

}

int main() {
    printf("Starting ITTM oracle simulation with blank tape...\n");
    initialize_tapes();
    setup_rules();
    simulate();
    print_tapes();
    print_halt_set();
    return 0;
}