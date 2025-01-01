#include "general.h"

// Function declarations
int ReadProgramFile(char* filename, int* IC, int* LC, int* WC);
int add_to_label_array(int LC, char* label_name, int address);
int add_to_word_array(int WC, char* instruction);
int add_to_instruction_array(int IC, char* instruction);
void trimWhitespace(char* str);
//void trimLeadingWhitespace(char* str);
void update_special_labels(int LC, int PC);
int search_char(char* str, char ch);
int search_str(char* str, char* s);