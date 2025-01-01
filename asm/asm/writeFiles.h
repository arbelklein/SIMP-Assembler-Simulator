#include "general.h"

// Function declarations
int WriteToFiles(char* instruction_filename, char* memory_filename, int inst_size, int mem_size);
int write_instructions(char* filename, int size);
int convert_opcode(char* opcode);
int convert_register(char* reg);
int write_memory(char* filename, int size);
int32_t search_word(int addr, int size);
int more_to_write(int size);
