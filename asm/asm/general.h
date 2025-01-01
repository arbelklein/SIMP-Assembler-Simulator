#ifndef General_H
#define General_H

#define _CRT_SECURE_NO_WARNINGS

//Includes for standard libraries
#include <stdint.h>
#include <stdio.h>

//Definitions
#define InstucMaxDepth 4096
#define MemoryMaxDepth 4096
#define MaxLines 500
#define MaxLabelLenth 50
#define MaxDataLength 35
#define MaxAddressLength 6
#define NumOfOperands 6
#define MaxOpcodeLength 5
#define MaxRegisterLength 6
#define NumOfRegsiters 16
#define NumOfOpcodes 22

//Defining structures
struct Instruction {
    char opcode[MaxOpcodeLength];
    char operands[NumOfOperands][MaxLabelLenth + 1];
    int usingLabel;
}; typedef struct Instruction Instruction;

struct Label {
    char lname[MaxLabelLenth + 1];
    int laddr;
}; typedef struct Label Label;

struct Word {
    int address;
    int32_t data;
    int wasWritten;
}; typedef struct Word Word;

//Global variables
extern char* registers_names[];
extern char* commands_names[];
extern Instruction instruc_arr[InstucMaxDepth];
extern Label label_arr[MaxLines];
extern Word word_arr[MaxLines];

//Function declaration
int classifyImmediate(char* imm);

#endif // !General_H