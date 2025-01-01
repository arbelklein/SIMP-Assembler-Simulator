#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


// Register Indexes
#define R_ZERO 0
#define IMM1 1
#define IMM2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7
#define R8 8
#define R9 9
#define R10 10
#define R11 11
#define R12 12
#define R13 13
#define R14 14
#define R15 15


// IO Register Indexes
#define IRQ_0_ENABLE_IOR 0
#define IRQ_1_ENABLE_IOR 1
#define IRQ_2_ENABLE_IOR 2
#define IRQ_0_STATUS_IOR 3 
#define IRQ_1_STATUS_IOR 4
#define IRQ_2_STATUS_IOR 5
#define IRQ_HANDLER_IOR 6
#define IRQ_RETURN_IOR 7
#define CLKS_IOR 8
#define LEDS_IOR 9
#define DISPLAY_7_SEG_IOR 10
#define TIMER_ENABLE_IOR 11
#define TIMER_CURRENT_IOR 12
#define TIMER_MAX_IOR 13
#define DISK_CMD_IOR 14
#define DISK_SECTOR_IOR 15
#define DISK_BUFFER_IOR 16
#define DISK_STATUS_IOR 17
#define RESERVED_1_IOR 18
#define RESERVED_2_IOR 19
#define MONITOR_ADDR_IOR 20
#define MONITOR_DATA_IOR 21
#define MONITOR_CMD_IOR 22

// Filname Indexes
#define SIM_F 0
#define IMEMIN_F 1
#define DMEMIN_F 2
#define DISKIN_F 3 
#define IRQ2IN_F 4
#define DMEMOUT_F 5
#define REGOUT_F 6
#define TRACE_F 7
#define HWREGTRACE_F 8
#define CYCLES_F 9
#define LEDS_F 10
#define DISPLAY7SEG_F 11
#define DISKOUT_F 12
#define MONITOR_TXT_F 13
#define MONITOR_YUV_F 14

 // irq2 struct
typedef struct {
    int current;
    int length;
    int* irq2Cycles;
} irq2_t;

// regNode contains (pc, instruction, registers) of current pc value
typedef struct regNode {
    uint16_t pc;
    uint64_t inst;
    int32_t regs[16];
    struct regNode* next;
} regNode;

// struct regList - tracing (pc, instruction, registers) data before each instruction line
typedef struct regList {
    regNode* head;
    regNode* tail;
} regList;

// hwRegNode contains (cycle, I/O operation, I/O registers) of current cycle
typedef struct hwRegNode {
    unsigned long cycle;
    char* RorW; // READ/WRITE
    char* IORegName;
    int32_t IORegData;
    struct hwRegNode* next;
} hwRegNode;

// struct hwRegList - tracing (cycle, I/O operation, I/O registers) every time I/O operation occures
typedef struct hwRegList {
    hwRegNode* head;
    hwRegNode* tail;
} hwRegList;

// trace struct - contains two lists for tracing (pc,instruction,registers) before each instruction line
// and (cycle, I/O operation, I/O registers) every time I/O operation occures
typedef struct trace {
    regList regs;
    hwRegList hwRegs;
} trace;


//Initialization
int prepare(char* args[]);

// Main functions
void executeInstruction();
void interruptServiceRoutine();

// Interrupt Service Routin auxilery functions
void copySector(int32_t* dest, int32_t* src);
void irq2Handler();
void diskHandler();
void timerHandler();

// CPU opcode functions
int32_t extendSign(int32_t num, int8_t signBitIndex); //An auxilery function
void add(int rd, int rs, int rt, int rm, int imm1, int imm2);
void sub(int rd, int rs, int rt, int rm, int imm1, int imm2);
void mac(int rd, int rs, int rt, int rm, int imm1, int imm2);
void and (int rd, int rs, int rt, int rm, int imm1, int imm2);
void or (int rd, int rs, int rt, int rm, int imm1, int imm2);
void xor (int rd, int rs, int rt, int rm, int imm1, int imm2);
void sll(int rd, int rs, int rt, int rm, int imm1, int imm2);
void sra(int rd, int rs, int rt, int rm, int imm1, int imm2);
void srl(int rd, int rs, int rt, int rm, int imm1, int imm2);
void beq(int rd, int rs, int rt, int rm, int imm1, int imm2);
void bne(int rd, int rs, int rt, int rm, int imm1, int imm2);
void blt(int rd, int rs, int rt, int rm, int imm1, int imm2);
void bgt(int rd, int rs, int rt, int rm, int imm1, int imm2);
void ble(int rd, int rs, int rt, int rm, int imm1, int imm2);
void bge(int rd, int rs, int rt, int rm, int imm1, int imm2);
void jal(int rd, int rs, int rt, int rm, int imm1, int imm2);
void lw(int rd, int rs, int rt, int rm, int imm1, int imm2);
void sw(int rd, int rs, int rt, int rm, int imm1, int imm2);
void reti(int rd, int rs, int rt, int rm, int imm1, int imm2);
void in(int rd, int rs, int rt, int rm, int imm1, int imm2);
void out(int rd, int rs, int rt, int rm, int imm1, int imm2);
void halt(int rd, int rs, int rt, int rm, int imm1, int imm2);

// Reading from input files
int readIrq2File();
int readDataAndInst();
int readDiskIn();

// Writing into output files
int writeToRegoutFile();
int writeToTraceFile();
int writeTohwRegs_Leds_display7Seg_Files();
int writeToDiskoutFile();
int writeToDmemoutFile();
int writeToMonitorFile();
int writeBinaryToMonitorFile();
int writeToCyclesFile();

// Updating traceOut
int updateHwRegsTrace(const char* RorW, int regIndex);
int updateRegsTrace();

// Finaliztion
int finalize();
int freeTrace();
int freeIrq2();
