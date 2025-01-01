#include "asm.h"

//Global variables
char* registers_names[] = { "$zero", "$imm1", "$imm2", "$v0", "$a0", "$a1", "$a2", "$t0", "$t1", "$t2", "$s0", "$s1", "$s2", "$gp", "$sp", "$ra" };
char* commands_names[] = { "add", "sub", "mac", "and", "or", "xor", "sll", "sra", "srl", "beq", "bne", "blt", "bgt", "ble", "bge", "jal", "lw", "sw", "reti", "in", "out", "halt" };
Instruction instruc_arr[InstucMaxDepth]; // Holds all the instructions
Label label_arr[MaxLines];               // Holds all the labels and their address
Word word_arr[MaxLines];                 // Holds all the address and data of the data memory

int main(int argc, char* argv[])
{
    char* program_filename, * instruction_filename, * memory_filename;
    int IC = 0, LC = 0, WC = 0;          // IC = instructions counter, LC = label counter, WC = word counter. used to navigate in the respectfully arrays


    if (argc != 4) // There aren't enough arguments
    {
        fprintf(stderr, "missing arguments");
        return 1;
    }

    program_filename = argv[1];     // The first argument is the filename of the program
    instruction_filename = argv[2]; // The second argument is the filename of the instruction memory image
    memory_filename = argv[3];      // The third argument is the filename of the data memory image

    // Reading the program for the first time
    if (ReadProgramFile(program_filename, &IC, &LC, &WC)) {
        fprintf(stderr, "Error reading the file\n");
        return 1;
    }

    // Updating the label's addresses (if exists) in the instructions
    if (UpdateLabels(IC, LC)) {
        fprintf(stderr, "Error parsing the labels\n");
        return 1;
    }

    // Writing the output files
    if (WriteToFiles(instruction_filename, memory_filename, IC, WC)) {
        fprintf(stderr, "Error writing to files\n");
        return 1;
    }

    return 0;
}