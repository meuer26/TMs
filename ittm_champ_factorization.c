// ITTM with Champernowne prime factorization


#include <stdio.h>    // For printf, fprintf: output functions
#include <stdlib.h>   // For atoi, exit: string to number, program exit
#include <stdint.h>   // For uint8_t, uint32_t: fixed-width integers
#include <string.h>   // For sprintf, memset: string and memory operations

// Configuration: ITTM oracle for teaching, using prime factorization of Champernowne numbers
#define MACHINES 32    // Number of tiny Turing machines to simulate
#define STATES 3       // States per machine: 0–1 for computation, 2 for halt
#define SYMBOLS 2      // Alphabet: 0, 1 (binary input for simulation)
#define STEPS 10000    // Steps: small approximation of infinite time (ω)
#define WINDOW 3       // Window size for loop detection (3 bits)
#define INPUT_LEN 5733 // Champernowne prefix: 1 to 1000 (~5733 chars)
#define MAX_PRIMES 25  // Primes <100 (2, 3, ..., 97)

// Machine structure: tracks state and position for each tiny TM
typedef struct {
    uint8_t state;      // Current state (0–2: 0–1 flip, 2 halt)
    uint32_t pos;       // Position on input tape (Tape 1)
    uint8_t done;       // 1 if halted (state=2) or looped
    uint32_t halt_step; // Step when halted or looped (for debugging)
} Machine;

// Global state: four tapes of the ITTM oracle
Machine machines[MACHINES];           // Tape 2: array of machine states
char input_tape[INPUT_LEN + 1];      // Tape 1: Champernowne prefix (string)
uint8_t sim_tape[MACHINES][WINDOW];  // Tape 3: simulation window for each machine
uint8_t halt_map[MACHINES / 8 + 1];  // Tape 4: 32-bit bitmap for halting set
uint8_t rules[MACHINES][STATES][SYMBOLS]; // Rules: transitions for each machine

// First 25 primes <100 for factorization
const uint32_t primes[MAX_PRIMES] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97
};

// Generate Champernowne prefix for Tape 1 (numbers 1 to 1000)
void load_champernowne() {
    int pos = 0; // Initialize position in input_tape
    // Loop through numbers 1 to 1000 to build Champernowne prefix
    for (int n = 1; n <= 1000 && pos < INPUT_LEN; n++) {
        char temp[5]; // Buffer for number (up to 4 digits + null)
        sprintf(temp, "%d", n); // Convert number n to string (e.g., 123 -> "123")
        for (int i = 0; temp[i] && pos < INPUT_LEN; i++) { // Copy each digit
            input_tape[pos++] = temp[i]; // Add digit to Tape 1
        }
    }
    input_tape[pos] = '\0'; // Null-terminate the string
    // Print first 50 chars of Tape 1 to show the prefix
    printf("Tape 1: Champernowne prefix = %.50s...\n", input_tape);
}

// Assign rules via prime factorization of Champernowne numbers (Tape 2)
void assign_rules() {
    // Parse Tape 1 to extract first 32 numbers
    int numbers[MACHINES]; // Array to store extracted numbers
    int count = 0, pos = 0; // Track number count and tape position
    char num_str[5] = {0}; // Buffer for building number strings (up to 4 digits)
    int num_pos = 0; // Position in num_str
    // Greedily parse digits into numbers (e.g., "123" -> 1, 2, 3)
    while (pos < INPUT_LEN && count < MACHINES) {
        if (input_tape[pos] >= '0' && input_tape[pos] <= '9') { // If digit
            num_str[num_pos++] = input_tape[pos]; // Add to number string
            // If 4 digits or end of tape, convert to number
            if (num_pos == 4 || input_tape[pos + 1] == '\0') {
                numbers[count++] = atoi(num_str); // Store number
                num_pos = 0; // Reset buffer position
                memset(num_str, 0, 5); // Clear buffer
            }
        }
        pos++; // Move to next tape position
    }
    // Define rule templates: halt-prone, cycle-prone, mixed
    uint8_t halt_prone[STATES][SYMBOLS] = {{1, 1}, {2, 2}, {0, 0}}; // [0->1,1] [1->2,2] [2->0,0]
    uint8_t cycle_prone[STATES][SYMBOLS] = {{1, 1}, {0, 0}, {0, 0}}; // [0->1,1] [1->0,0] [2->0,0]
    uint8_t mixed[STATES][SYMBOLS] = {{1, 1}, {2, 0}, {0, 0}};      // [0->1,1] [1->2,0] [2->0,0]
    // Assign rules for each of 32 machines using prime factorization
    for (int i = 0; i < MACHINES; i++) {
        machines[i].state = 0; // Initialize state to 0 (running)
        machines[i].pos = 0;   // Start at tape position 0
        machines[i].done = 0;  // Not halted or looped
        machines[i].halt_step = 0; // No termination yet
        // Zero out Tape 3 for loop detection
        for (int j = 0; j < WINDOW; j++) sim_tape[i][j] = 0;
        // Get number for machine (default to i+1 if not enough)
        int num = (count > i) ? numbers[i] : i + 1;
        // Factorize number using primes <100
        int factor_count = 0; // Count distinct prime factors <100
        char factor_str[50] = {0}; // Buffer for factor string
        int factor_pos = 0; // Position in factor_str
        int temp = num, prime_idx = 0; // Track factorization
        while (temp > 1 && prime_idx < MAX_PRIMES) {
            int count = 0; // Count occurrences of prime
            while (temp % primes[prime_idx] == 0) { // Divide by prime
                count++; // Increment exponent
                temp /= primes[prime_idx]; // Reduce number
            }
            if (count > 0) { // If prime was used
                factor_count++; // Increment factor count
                // Append prime^count to factor string
                char temp_str[10];
                sprintf(temp_str, "%d^%d × ", primes[prime_idx], count);
                for (int k = 0; temp_str[k] && factor_pos < 49; k++) {
                    factor_str[factor_pos++] = temp_str[k];
                }
            }
            prime_idx++; // Move to next prime
        }
        if (temp > 1) { // Remaining factor (prime >97 or number itself)
            char temp_str[10];
            sprintf(temp_str, "%d", temp);
            for (int k = 0; temp_str[k] && factor_pos < 49; k++) {
                factor_str[factor_pos++] = temp_str[k];
            }
        } else if (factor_count == 0) { // Number 1 or prime >97
            sprintf(factor_str, "%d", num);
            factor_count = 1; // Treat as one factor
        } else { // Remove trailing " × "
            if (factor_pos >= 3) factor_pos -= 3;
        }
        factor_str[factor_pos] = '\0'; // Null-terminate factor string
        // Assign rule template based on factor count mod 4
        int rule_idx = factor_count % 4; // 0,1: cycle-prone, 2: mixed, 3: halt-prone
        for (int s = 0; s < STATES; s++) {
            for (int sym = 0; sym < SYMBOLS; sym++) {
                if (rule_idx == 0 || rule_idx == 1) {
                    rules[i][s][sym] = cycle_prone[s][sym];
                } else if (rule_idx == 2) {
                    rules[i][s][sym] = mixed[s][sym];
                } else {
                    rules[i][s][sym] = halt_prone[s][sym];
                }
            }
        }
        // Print number, full factorization, and actual rules for debugging
        printf("Machine %d: Number=%d, Factorization=%s, Rules=[0->%d,%d] [1->%d,%d] [2->%d,%d]\n",
               i, num, factor_str,
               rules[i][0][0], rules[i][0][1],
               rules[i][1][0], rules[i][1][1],
               rules[i][2][0], rules[i][2][1]);
    }
    // Zero out Tape 4 for halting set
    for (int i = 0; i < MACHINES / 8 + 1; i++) {
        halt_map[i] = 0; // Clear halt bitmap
    }
    // Print rules for machine 0 for teaching clarity
    printf("Rules for machine 0: [0->%d,%d] [1->%d,%d] [2->%d,%d]\n",
           rules[0][0][0], rules[0][0][1], rules[0][1][0], rules[0][1][1],
           rules[0][2][0], rules[0][2][1]);
    // Confirm rule generation completed
    printf("Rule generation completed for all machines.\n");
}

// Check for loops in Tape 3 (e.g., repeating "0", "00", or "11")
// Mimics ITTM loop detection at ω steps
int check_loop(int m, uint32_t step) {
    if (step < 30) return 0; // Wait for 30 steps to check loops
    // Check for period=1 to 3 loops (e.g., "0", "00", "11")
    for (int period = 1; period <= WINDOW; period++) {
        int match = 1; // Assume loop
        for (int i = 0; i < period; i++) { // Compare symbols
            int idx1 = (step - i - 1) % WINDOW; // Recent symbol
            int idx2 = (step - i - 1 - period) % WINDOW; // Earlier symbol
            if (sim_tape[m][idx1] != sim_tape[m][idx2]) { // No match
                match = 0; // Not a loop
                break;
            }
        }
        if (match) return 1; // Loop detected
    }
    return 0; // No loop detected
}

// Dovetail: run all machines like an ITTM oracle
// Tape 3: simulates TMs; Tape 4: records halts
void dovetail() {
    // Run for 10,000 steps, a small slice of infinite time
    for (uint32_t step = 1; step <= STEPS; step++) {
        // Process machines 0 to min(step-1, 31)
        for (int m = 0; m < MACHINES && m < step; m++) {
            if (machines[m].done) continue; // Skip finished machines
            // Read Tape 1 digit, convert to 0/1 (mod 2)
            uint8_t sym = (input_tape[machines[m].pos % INPUT_LEN] - '0') % 2;
            // Write to Tape 3 (simulation window)
            sim_tape[m][step % WINDOW] = sym;
            // Apply rule to get next state
            uint8_t next = rules[m][machines[m].state][sym];
            machines[m].state = next; // Update state
            machines[m].pos++; // Move tape position
            // Check for halt (state=2) or loop
            if (next == 2 || (step >= 30 && check_loop(m, step))) {
                machines[m].done = 1; // Mark as done
                if (next == 2) { // If halted
                    halt_map[m / 8] |= (1 << (m % 8)); // Set Tape 4 bit
                    machines[m].halt_step = step; // Record halt step
                } else { // If looped
                    machines[m].halt_step = step; // Record loop step
                }
            }
        }
    }
}

// Print Tape 4: halting set prefix
// Shows which machines halted (1) or looped (0)
void print_halt_map() {
    int halts = 0;
    // Print Tape 4 as a 32-bit bitmap
    printf("Tape 4 (1=halted):\n");
    for (int i = 0; i < MACHINES; i++) {
        int bit = (halt_map[i / 8] >> (i % 8)) & 1; // Get bit (0 or 1)
        printf("%d", bit); // Print bit
        halts += bit; // Count halts
        if (i % 8 == 7) printf(" "); // Space every 8 bits for readability
    }
    // Print total halts
    printf("\nHalted: %d/%d\n", halts, MACHINES);
    // Debug: show termination steps for first 5 machines
    printf("Termination steps for first 5 machines:\n");
    for (int m = 0; m < 5 && m < MACHINES; m++) {
        if (machines[m].done && (halt_map[m / 8] & (1 << (m % 8)))) {
            printf("Machine %d halted at step %u\n", m, machines[m].halt_step);
        } else if (machines[m].done) {
            printf("Machine %d looped at step %u\n", m, machines[m].halt_step);
        } else {
            printf("Machine %d still running at step %u\n", m, STEPS);
        }
    }
}

int main() {
    // Start the ITTM oracle simulation
    printf("Starting ITTM oracle simulation with Champernowne...\n");
    load_champernowne(); // Tape 1: generate Champernowne prefix
    assign_rules(); // Tape 2: parse and set rules via prime factorization
    dovetail(); // Tape 3: run dovetailed simulation
    print_halt_map(); // Tape 4: show halting set prefix
    return 0; // Exit program
}