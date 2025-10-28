// ITTM with variable 3 state TMs and dovetailing

#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Configuration: ITTM simulation for teaching, up to 30 three-state machines
#define MAX_MACHINES 30
#define NUM_STATES 3
#define NUM_SYMBOLS 2
#define MAX_STEPS 500
#define MAX_PERSONAL_STEPS 100
#define WINDOW_SIZE 20
#define TAPE_LENGTH 1000

// Structure for each Turing machine
typedef struct {
    uint8_t current_state;   // Current state (0, 1, or 2 for halt)
    uint32_t tape_position;  // Position on input tape
    uint8_t halted;          // 1 if halted or looped
    uint32_t halt_step;      // Personal step count at which machine halted or looped
    uint8_t tape[TAPE_LENGTH]; // Individual input tape
} TuringMachine;

// Structure for transition rules
typedef struct {
    uint8_t write_symbol;    // Symbol to write (0 or 1)
    uint8_t next_state;      // Next state (0, 1, or 2)
} Rule;

// Global state
TuringMachine tms[MAX_MACHINES];              // Tape 2: machine states
uint8_t output_tape[MAX_MACHINES][WINDOW_SIZE]; // Tape 3: simulation window
uint8_t halt_set[(MAX_MACHINES / 8) + 1];    // Tape 4: halting set bitmap
Rule rule_table[MAX_MACHINES][NUM_STATES][NUM_SYMBOLS];

// Initialize each machine's input tape to all 0s, except Machine 9
void initialize_tapes(int num_machines) {
    for (int i = 0; i < num_machines; i++) {
        memset(tms[i].tape, 0, TAPE_LENGTH);
        if (i == 9) tms[i].tape[1] = 1; // Add a 1 for Machine 9 to trigger state transition
    }
    printf("Tape 1: Blank input = %.50s...\n", tms[0].tape);
    if (num_machines > 9) {
        printf("Tape 1 (Machine 9) = %.50s...\n", tms[9].tape);
    }
}

// Set up rules for machines (cycling through 20 balanced templates)
void setup_rules(int num_machines) {
    // Define 20 balanced rule templates (10 looping, 10 halting)
    const Rule templates[20][NUM_STATES][NUM_SYMBOLS] = {
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

    const char* descriptions[20] = {
        "Loop (stay in state 0)",
        "Halt after two steps",
        "Loop (cycle 0<->1)",
        "Loop (write 1, stay 1)",
        "Halt after two steps",
        "Halt immediately",
        "Loop (write 0, stay 1)",
        "Halt after two steps",
        "Halt immediately",
        "Halt after three steps",
        "Loop (write 1, cycle 0<->1)",
        "Halt immediately",
        "Halt after two steps",
        "Halt immediately",
        "Loop (stay in state 0)",
        "Loop (stay in state 0)",
        "Loop (write 0, cycle 0<->1)",
        "Halt immediately",
        "Loop (write 0, cycle 0<->1)",
        "Loop (write 0, cycle 0<->1)"
    };

    // Initialize machines and copy rules
    for (int i = 0; i < num_machines; i++) {
        int pat = i % 20;
        tms[i].current_state = 0;
        tms[i].tape_position = 0;
        tms[i].halted = 0;
        tms[i].halt_step = 0;
        for (int j = 0; j < WINDOW_SIZE; j++) output_tape[i][j] = 0;
        for (int s = 0; s < NUM_STATES; s++) {
            for (int sym = 0; sym < NUM_SYMBOLS; sym++) {
                rule_table[i][s][sym] = templates[pat][s][sym];
            }
        }
        printf("Machine %d: Rules=[0->%d,%d] [1->%d,%d] [0->%d,%d] %s\n",
               i, rule_table[i][0][0].write_symbol, rule_table[i][0][0].next_state,
               rule_table[i][0][1].write_symbol, rule_table[i][0][1].next_state,
               rule_table[i][1][0].write_symbol, rule_table[i][1][0].next_state,
               descriptions[pat]);
    }
    memset(halt_set, 0, sizeof(halt_set));
    printf("Rule generation completed for all machines.\n");
}

// Check for loops in Tape 3 and state periodicity
int detect_loop(int machine_idx, uint32_t step) {
    // Update state buffer
    static uint8_t past_states[MAX_MACHINES][WINDOW_SIZE];
    static uint32_t state_steps[MAX_MACHINES][WINDOW_SIZE];
    past_states[machine_idx][(step - 1) % WINDOW_SIZE] = tms[machine_idx].current_state;
    state_steps[machine_idx][(step - 1) % WINDOW_SIZE] = step;

    if (step < MAX_PERSONAL_STEPS) return 0; // Threshold for loop detection
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
    return 0;
}

// Check if all machines are halted
int all_machines_halted(int num_machines) {
    for (int m = 0; m < num_machines; m++) {
        if (!tms[m].halted) return 0;
    }
    return 1;
}

// Print aligned header for machine states
void print_header() {
    printf("%-8s %-6s %-6s %-5s %-10s %s\n",
           "Machine", "State", "Pos", "Done", "HaltStep", "Tape3");
}

// Print aligned row for a machine
void print_machine_row(int i) {
    int last_j = (tms[i].halt_step > 0) ? ((int)tms[i].halt_step - 1) % WINDOW_SIZE : -1;
    char tape_str[100]; // Buffer for Tape3 string
    int pos = sprintf(tape_str, "[");
    for (int j = 0; j < WINDOW_SIZE; j++) {
        if (j > 0) {
            pos += sprintf(tape_str + pos, ",");
        }
        if (last_j >= 0 && j == last_j) {
            pos += sprintf(tape_str + pos, "[%d]", output_tape[i][j]); // Highlight with *
        } else {
            pos += sprintf(tape_str + pos, "%d", output_tape[i][j]);
        }
    }
    pos += sprintf(tape_str + pos, "]");
    tape_str[pos] = '\0';

    printf("%-8d %-6d %-6u %-5d %-10u %s\n",
           i, tms[i].current_state, tms[i].tape_position, tms[i].halted, tms[i].halt_step, tape_str);
}

// Simulate all machines in dovetailed fashion with pause after each stage
void simulate(int num_machines) {
    for (uint32_t stage = 1; stage <= MAX_STEPS; stage++) {
        printf("Stage %u:\n", stage);
        // Perform one step for machines 0 to min(stage-1, num_machines-1)
        for (int m = 0; m < num_machines && m < stage; m++) {
            if (tms[m].halted) continue;
            uint32_t personal_step = tms[m].halt_step + 1;
            uint8_t symbol = tms[m].tape[tms[m].tape_position % TAPE_LENGTH];
            uint8_t write = rule_table[m][tms[m].current_state][symbol].write_symbol;
            uint8_t next = rule_table[m][tms[m].current_state][symbol].next_state;
            printf("Machine %d: Personal step %u (global stage %u), Read %d, Write %d, Next State %d\n",
                   m, personal_step, stage, symbol, write, next);
            // Update circular window with this write
            output_tape[m][tms[m].halt_step % WINDOW_SIZE] = write;
            tms[m].tape[tms[m].tape_position % TAPE_LENGTH] = write;
            tms[m].current_state = next;
            tms[m].tape_position++;
            tms[m].halt_step = personal_step;
            if (next == 2 || (personal_step >= MAX_PERSONAL_STEPS && detect_loop(m, personal_step))) {
                tms[m].halted = 1;
                if (next == 2) {
                    halt_set[m / 8] |= (1 << (m % 8));
                }
            }
        }
        // Print Tape 2 and Tape 3 combined for each machine with alignment
        printf("Machine States and Simulation Window:\n");
        print_header();
        for (int i = 0; i < num_machines; i++) {
            print_machine_row(i);
        }
        // Check if all halted
        if (all_machines_halted(num_machines)) break;
        // Pause and wait for key press (Enter)
        printf("Press Enter to continue...\n");
        getchar(); // Wait for Enter key
    }
}

// Print Tape 2 and Tape 3 combined with alignment
void print_tapes(int num_machines) {
    printf("Final Machine States and Simulation Window:\n");
    print_header();
    for (int i = 0; i < num_machines; i++) {
        print_machine_row(i);
    }
}

// Print Tape 4: halting set
void print_halt_set(int num_machines) {
    int halts = 0;
    printf("Tape 4 (1=halted):\n");
    for (int i = 0; i < num_machines; i++) {
        int bit = (halt_set[i / 8] >> (i % 8)) & 1;
        printf("%d", bit);
        halts += bit;
        if (i % 8 == 7) printf(" ");
    }
    printf("\nHalted: %d/%d\n", halts, num_machines);
}

int main() {
    int num_machines;
    printf("Enter number of machines (1-30): ");
    scanf("%d", &num_machines);
    if (num_machines < 1 || num_machines > MAX_MACHINES) {
        printf("Invalid number of machines. Using 20.\n");
        num_machines = 20;
    }
    printf("Starting ITTM oracle simulation with %d machines and blank tape...\n", num_machines);
    initialize_tapes(num_machines);
    setup_rules(num_machines);
    printf("\nSimulation ready. Press Enter to begin...\n");
    getchar();
    simulate(num_machines);
    print_tapes(num_machines);
    print_halt_set(num_machines);
    return 0;
}