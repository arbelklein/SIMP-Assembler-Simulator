#define _CRT_SECURE_NO_WARNINGS
#include "sim.h"

// General macros
#define SUCCESS_CODE 0
#define FAIL_CODE 1
#define HEX_LEN 4
#define FILES_AMOUNT 15
#define REGS_SIZE 16
#define IOREGS_SIZE 23
#define OPCODES_AMOUNT 22

// Error messeges
#define OPEN_ERR "Error - Failed opening a file\n"
#define ERR_ALLOC_MEM "Error - Faild allocating memory\n"

//For instructions handling
#define INSTRUCTION_LINE_LENGTH 12 // 12 hex digits
#define INSTRUCTION_MAX_LINES 4096

//For memory handling
#define MEMORY_LINE_LENGTH 8       // 8 hex digits
#define MEMORY_MAX_LINES 4096
#define IMMEDIATE_WIDTH 12 


//For disk handling
#define DISK_SECTOR_SIZE 128 // one sector size is 128 lines, each line 8 hexa digits - 4 bytes.
#define DISK_MAX_SECTORS 128
#define MAX_DISK_CYCLES 1024

//For monitor handling
#define MONITOR_SIZE 256


//------------------- Data structure -----------------------
// General 
trace traceOut;
const char* fileNames[FILES_AMOUNT];
const char* IORegsNames[] = { "irq0enable","irq1enable","irq2enable","irq0status","irq1status","irq2status","irqhandler","irqreturn", "clks","leds","display7seg","timerenable","timercurrent","timermax","diskcmd","disksector","diskbuffer","diskstatus","reserved","reserved","monitoraddr","monitordata","monitorcmd" };
unsigned long cycles;

//Registers
int32_t Regs[REGS_SIZE]; // index represents encoding of the register, value is the register value
int32_t IORegs[IOREGS_SIZE];

//For executeInstruction method
uint64_t instructions[INSTRUCTION_MAX_LINES]; //only 48 lowest bits are count
void (*execute[OPCODES_AMOUNT])() = { add, sub, mac, and, or , xor, sll, sra, srl, beq, bne, blt, bgt, ble, bge, jal, lw, sw, reti, in, out, halt };
int pc;
int toHalt;


//For memory handling
int32_t memory[MEMORY_MAX_LINES];
int memCurLen;

//For disk handling
int32_t disk[DISK_MAX_SECTORS][DISK_SECTOR_SIZE];
int diskCycle;
int diskCurLen;

//For interruptServiceRoutine() method
irq2_t irq2;
int isrBusy;
int irq;

//For Monitor handling
uint8_t monitor[MONITOR_SIZE * MONITOR_SIZE];
int monitorCurLen;

//------------------- Initialization -----------------------
// Initialize data structure and load files into into it
int prepare(char* args[])
{
    int i;
    for (i = 0; i < FILES_AMOUNT; i++)
        fileNames[i] = args[i];
    pc = 0;
    cycles = 0;
    isrBusy = 0;
    diskCycle = 0;
    toHalt = 0;
    memCurLen = 0;
    diskCurLen = 0;
    monitorCurLen = 0;

    irq2.current = 0;
    irq2.length = 0;
    irq2.irq2Cycles = NULL;

    if (FAIL_CODE == (readDiskIn() | readIrq2File() | readDataAndInst())) {
        perror("Error in loading input files\n");
        return FAIL_CODE;
    }


    traceOut.regs.head = NULL;
    traceOut.regs.tail = NULL;
    traceOut.hwRegs.head = NULL;
    traceOut.hwRegs.tail = NULL;

    return SUCCESS_CODE;
}

//------------------- Main functions -----------------------

// Update the given pointers with new values according to instruction line
void executeInstruction()
{
    int imm2, imm1, rm, rt, rs, rd, opcode;
    // Parse instruction
    uint64_t inst = instructions[pc];
    imm2 = inst & 0xFFF;
    imm1 = (inst >> (3 * HEX_LEN)) & 0xFFF;
    rm = (inst >> (2 * (3 * HEX_LEN))) & 0xF;
    rt = (inst >> (HEX_LEN + 2 * (3 * HEX_LEN))) & 0xF;
    rs = (inst >> (2 * HEX_LEN + 2 * (3 * HEX_LEN))) & 0xF;
    rd = (inst >> (3 * HEX_LEN + 2 * (3 * HEX_LEN))) & 0xF;
    opcode = (inst >> (4 * HEX_LEN + 2 * (3 * HEX_LEN)));

    // Zero register and immediates should do not change their values so if insert to Regs[rd] will happen only if rd>2
    Regs[R_ZERO] = 0;
    Regs[IMM1] = extendSign(imm1, IMMEDIATE_WIDTH - 1); //Immediates should contain the immediate values with signed extention inside register array
    Regs[IMM2] = extendSign(imm2, IMMEDIATE_WIDTH - 1);

    updateRegsTrace(); // Update trace after zero and immediates are updated and before each execution

    //execute
    execute[opcode](rd, rs, rt, rm, imm1, imm2); // Run the opcode with given params encoded in instruction line
}

// ISR - Interrupt Service Routine
void interruptServiceRoutine() {
    //Update statuses of former interrupts andcheck for new interrupt requests
    timerHandler();
    diskHandler();
    irq2Handler();

    // Check is statuses and enables had changed requiring an interrupt
    irq = (IORegs[IRQ_0_ENABLE_IOR] & IORegs[IRQ_0_STATUS_IOR]) || (IORegs[IRQ_1_ENABLE_IOR] & IORegs[IRQ_1_STATUS_IOR]) || (IORegs[IRQ_2_ENABLE_IOR] & IORegs[IRQ_2_STATUS_IOR]);

    // If interrupt was required - save current pc, jump into interrupt handler line and change isrBusy global variable to "busy".
    if (irq == 1 && isrBusy == 0)
    {
        isrBusy = 1; //Interrupt occured
        IORegs[IRQ_RETURN_IOR] = pc; // Save pc to coninue from where we started (before the launch of current cycle code line)
        pc = IORegs[IRQ_HANDLER_IOR]; // Go to interrupt code
    }

    // Update IORegs and drivers after each cycle:
    IORegs[CLKS_IOR]++;
}

//------------------- Interrupt Service Routin auxilery functions -----------------------

//Copies 128 lines of int32_t from disk to memory or from memmory to disk
void copySector(int32_t* dest, int32_t* src)
{
    int offset;
    for (offset = 0; offset < DISK_SECTOR_SIZE; offset++)
    {
        dest[offset] = src[offset];
        //Update length of disk where data != 0
        if (IORegs[DISK_CMD_IOR] == 1 && dest[offset] != 0 && memCurLen <= IORegs[DISK_BUFFER_IOR] + offset)
            memCurLen = IORegs[DISK_BUFFER_IOR] + offset + 1;
        if (IORegs[DISK_CMD_IOR] == 2 && dest[offset] != 0 && diskCurLen <= IORegs[DISK_SECTOR_IOR] * DISK_SECTOR_SIZE + offset)
            diskCurLen = IORegs[DISK_SECTOR_IOR] * DISK_SECTOR_SIZE + offset + 1;
    }
}

//Handles Disk interrupt
void diskHandler() {
    if (IORegs[DISK_CMD_IOR] == 1 || IORegs[DISK_CMD_IOR] == 2 || IORegs[DISK_STATUS_IOR] == 1) {
        if (diskCycle == MAX_DISK_CYCLES)
        {
            //Last disk cycle - disk will now be free to recieve new requests
            IORegs[DISK_STATUS_IOR] = 0;
            IORegs[DISK_CMD_IOR] = 0;
            IORegs[IRQ_1_STATUS_IOR] = 1; //disk noify an interrupt when finished 1024 cycles of work
            diskCycle = 0; //next cycle where IORegs[DISK_CMD_IOR]=1 is going to be the first disk cycle
        }
        else {
            IORegs[DISK_STATUS_IOR] = 1;
            if (diskCycle == 0) {

                // First disk cycle - make a disk operation only in this cycle
                if (IORegs[DISK_CMD_IOR] == 1) {//read from disk to memory 
                    copySector(&(memory[IORegs[DISK_BUFFER_IOR]]), disk[IORegs[DISK_SECTOR_IOR]]);
                }
                else if (IORegs[DISK_CMD_IOR] == 2) { //write from memory to disk
                    copySector(disk[IORegs[DISK_SECTOR_IOR]], &(memory[IORegs[DISK_BUFFER_IOR]]));

                }
            }
            diskCycle++;
        }
    }
}

//Handle Timer interrupt
void timerHandler() {
    // Hanldle only if timer enable is on
    if (IORegs[TIMER_ENABLE_IOR]) {
        if (IORegs[TIMER_CURRENT_IOR] == IORegs[TIMER_MAX_IOR]) {
            IORegs[TIMER_CURRENT_IOR] = 0;
            IORegs[IRQ_0_STATUS_IOR] = 1; //Timer noify an interrupt when getting to max value cycles of work
        }
        else
            IORegs[TIMER_CURRENT_IOR]++; // Increase timer
    }

}

//Handles irq2 interrupt
void irq2Handler() {
    if (irq2.current == irq2.length)
        return; // Then we already done all irq2 interrupts
    if (irq2.irq2Cycles[irq2.current] == cycles) {
        IORegs[IRQ_2_STATUS_IOR] = 1;
        irq2.current++; // Set next cycle to get irq2
    }
}


//------------------- CPU opcode functions -----------------------

//An auxiliary function - Performs sigend extention to int32_t given the requested bit to extend left
int32_t extendSign(int32_t num, int8_t signBitIndex)
{
    int sign = (num >> signBitIndex) & 1; // gets sign of signBitIndex bit
    if (sign) //If sign bit is 1 then perfom signed extention
        num = num | (0xFFFFFFFF << (signBitIndex + 1));  // Set all 32 bits to 1 then the sign bit and the lower bits to 0, then perform bitwise or with original number
    //If sign=0 then num is positive so already in sigened extention format
    return num;
}

// Arithmetic functions
void add(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    Regs[rd] = Regs[rs] + Regs[rt] + Regs[rm];
    pc++;
}
void sub(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    Regs[rd] = Regs[rs] - Regs[rt] - Regs[rm];
    pc++;
}
void mac(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    Regs[rd] = Regs[rs] * Regs[rt] + Regs[rm];
    pc++;
}
void and (int rd, int rs, int rt, int rm, int imm1, int imm2) {
    Regs[rd] = Regs[rs] & Regs[rt] & Regs[rm];
    pc++;
}
void or (int rd, int rs, int rt, int rm, int imm1, int imm2) {
    Regs[rd] = Regs[rs] | Regs[rt] | Regs[rm];
    pc++;
}
void xor (int rd, int rs, int rt, int rm, int imm1, int imm2) {
    Regs[rd] = Regs[rs] ^ Regs[rt] ^ Regs[rm];
    pc++;
}
void sll(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    Regs[rd] = Regs[rs] << Regs[rt];
    pc++;
}
void sra(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    Regs[rd] = Regs[rs] >> Regs[rt];
    Regs[rd] = extendSign(Regs[rd], 31 - Regs[rt]); // shift logical right and extend the msb to the lefet from its new posion
    pc++;
}
void srl(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    Regs[rd] = Regs[rs] >> Regs[rt];
    pc++;
}

// Jump functions
void beq(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    if (Regs[rs] == Regs[rt])
        pc = Regs[rm] & 0xFFF;
    else
        pc++;
}
void bne(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    if (Regs[rs] != Regs[rt])
        pc = Regs[rm] & 0xFFF;
    else
        pc++;
}
void blt(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    if (Regs[rs] < Regs[rt])
        pc = Regs[rm] & 0xFFF;
    else
        pc++;
}
void bgt(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    if (Regs[rs] > Regs[rt])
        pc = Regs[rm] & 0xFFF;
    else
        pc++;
}
void ble(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    if (Regs[rs] <= Regs[rt])
        pc = Regs[rm] & 0xFFF;
    else
        pc++;
}
void bge(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    if (Regs[rs] >= Regs[rt])
        pc = Regs[rm] & 0xFFF;
    else
        pc++;
}
void jal(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    Regs[rd] = pc + 1; //save next instruction address
    pc = Regs[rm] & 0xFFF; // jump to requested address
}

// Memory functions
void lw(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    Regs[rd] = memory[Regs[rs] + Regs[rt]] + Regs[rm];
    pc++;
}
void sw(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    memory[(Regs[rs] + Regs[rt]) & 0xFFF] = Regs[rd] + Regs[rm];
    if (((Regs[rs] + Regs[rt]) & 0xFFF) >= memCurLen && (Regs[rd] + Regs[rm]) != 0)
        memCurLen = ((Regs[rs] + Regs[rt]) & 0xFFF) + 1; //update length of used memory
    pc++;
}

// IO opcode functions
void reti(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    pc = IORegs[IRQ_RETURN_IOR] & 0xfff; // return back from interrupt to the code line before the interrupt was triggered
    isrBusy = 0; // Assembly code finished interrupt task - isr is now ready for next interrupts (disk may still be working and not ready)
}
void in(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    Regs[rd] = IORegs[Regs[rs] + Regs[rt]];
    pc++;
    updateHwRegsTrace("READ", Regs[rs] + Regs[rt]); //Recored reading from HwRegs file
}
void out(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    IORegs[Regs[rs] + Regs[rt]] = Regs[rm];
    updateHwRegsTrace("WRITE", Regs[rs] + Regs[rt]); //Recored writing from HwRegs file

    if (IORegs[MONITOR_CMD_IOR] == 1) {
        monitor[IORegs[MONITOR_ADDR_IOR]] = IORegs[MONITOR_DATA_IOR]; // write pixel to display 
        IORegs[MONITOR_CMD_IOR] = 0; // Reading monitorcmd with in command should returns 0
        if (IORegs[MONITOR_ADDR_IOR] >= monitorCurLen && IORegs[MONITOR_DATA_IOR] != 0)
            monitorCurLen = IORegs[MONITOR_ADDR_IOR] + 1; // Update length of used memory in monitor
    }
    pc++;
}
void halt(int rd, int rs, int rt, int rm, int imm1, int imm2) {
    toHalt = 1;
}

//------------------- Reading from input files -----------------------

// Reads disk input file into 2D array disk[sector][offset]
int readDiskIn()
{
    int sector, offset;
    FILE* diskIn;

    // Open disk input file
    if ((diskIn = fopen(fileNames[DISKIN_F], "r")) == NULL) {
        perror(OPEN_ERR);
        return FAIL_CODE;
    }

    // Read disk input file into disk 2D array
    for (sector = 0; sector < DISK_MAX_SECTORS; sector++) {
        for (offset = 0; offset < DISK_SECTOR_SIZE; offset++) {
            if (fscanf(diskIn, "%X", &(disk[sector][offset])) != 1) //X meaning hexa base format with capital letters
                continue;
            if (disk[sector][offset] != 0) { // update max line that not equal 0.
                diskCurLen = sector * DISK_SECTOR_SIZE + offset + 1; // update length of used memory in disk
            }
        }
    }
    fclose(diskIn);
    return SUCCESS_CODE;
}

// Reads instructions and memory from input file into array
int readDataAndInst()
{
    int offset;
    memCurLen = 0;
    FILE* dmemFile, * instFile;

    // Open memory input file
    if ((dmemFile = fopen(fileNames[DMEMIN_F], "r")) == NULL) {
        perror(OPEN_ERR);
        return FAIL_CODE;
    }
    // Open instructions file file
    if ((instFile = fopen(fileNames[IMEMIN_F], "r")) == NULL) {
        perror(OPEN_ERR);
        return FAIL_CODE;
    }
    // Read memory input file into memory array
    for (offset = 0; offset < MEMORY_MAX_LINES; offset++) {
        if ((fscanf(dmemFile, "%x", &(memory[offset]))) != 1)
            break;
        if (memory[offset] != 0)
            memCurLen = offset + 1;
    }

    //Read instructions input file into instructions array
    for (offset = 0; offset < INSTRUCTION_MAX_LINES; offset++)
        if ((fscanf(instFile, "%llx", &(instructions[offset]))) != 1)
            break;

    fclose(dmemFile);
    fclose(instFile);

    return SUCCESS_CODE;
}

// Given returns array of strings - each s tring is a line in the file
int readIrq2File()
{
    int cycle;

    // Open irq2 input file
    FILE* irq2File = fopen(fileNames[IRQ2IN_F], "r");
    if (irq2File == NULL) {
        perror("Error opening file");
        return FAIL_CODE;
    }

    // Allocate array of increasing length until last integer is read from the file
    int length = 0;
    int* cyclesArr = NULL;
    int* tmpArr = NULL;
    while (fscanf(irq2File, "%d", &cycle) == 1) {
        length++;
        tmpArr = realloc(cyclesArr, length * sizeof(int));
        if (tmpArr == NULL) {
            perror(ERR_ALLOC_MEM);
            return FAIL_CODE;
        }
        cyclesArr = tmpArr;
        cyclesArr[length - 1] = cycle; // Next free cell in array is always the last index
    }
    fclose(irq2File);

    //Initialize irq2 with loaded data
    irq2.irq2Cycles = cyclesArr;
    irq2.length = length;
    irq2.current = 0;
    return SUCCESS_CODE;
}

//------------------- Updating traceOut -----------------------

// Adds a node to traceOut.regs list, each node contains pc, instruction, and Regs values
int updateRegsTrace() {
    regNode* newRegNode;
    int i;

    // Create new regNode
    if ((newRegNode = (regNode*)malloc(sizeof(regNode))) == NULL) {
        perror(ERR_ALLOC_MEM);
        return FAIL_CODE;
    }
    newRegNode->pc = pc;
    newRegNode->inst = instructions[pc];
    newRegNode->next = NULL;
    for (i = 0; i < REGS_SIZE; i++)
        newRegNode->regs[i] = Regs[i];

    //Insert node into traceOut.regs list
    if (traceOut.regs.head == NULL) {
        traceOut.regs.head = newRegNode;
        traceOut.regs.tail = traceOut.regs.head;
    }
    else {
        traceOut.regs.tail->next = newRegNode;
        traceOut.regs.tail = traceOut.regs.tail->next;
    }
    return SUCCESS_CODE;
}

// Adds a node to traceOut.regs list, each node contains cycle, I/O instruction (READ or WRITE),
// IOReg that has been read or written, and IORegs values
int updateHwRegsTrace(const char* RorW, int regIndex) {
    hwRegNode* newHwRegsNode;

    // Create new hwRegNode
    if ((newHwRegsNode = (hwRegNode*)malloc(sizeof(hwRegNode))) == NULL) {
        perror(ERR_ALLOC_MEM);
        return FAIL_CODE;
    }
    newHwRegsNode->cycle = cycles;
    newHwRegsNode->RorW = _strdup(RorW); // Allocate memory for RorW string and copy it
    newHwRegsNode->IORegName = _strdup(IORegsNames[regIndex]); // Allocate memory for IORegData string and copy it
    newHwRegsNode->IORegData = IORegs[regIndex];

    //Insert node into traceOut.hwRegs list
    newHwRegsNode->next = NULL;
    if (traceOut.hwRegs.head == NULL) {
        traceOut.hwRegs.head = newHwRegsNode;
        traceOut.hwRegs.tail = traceOut.hwRegs.head;
    }
    else {
        traceOut.hwRegs.tail->next = newHwRegsNode;
        traceOut.hwRegs.tail = traceOut.hwRegs.tail->next;
    }
    return SUCCESS_CODE;
}



//------------------- Writing into output files -----------------------

// Writes to monitor.yuv the monitor data from monitor array in binary format
int writeBinaryToMonitorFile() {
    FILE* file;

    // Open in monitor out put file .yuv in binary- write mode
    file = fopen(fileNames[MONITOR_YUV_F], "wb");
    if (file == NULL)
    {
        perror(OPEN_ERR);
        return FAIL_CODE;
    }

    // Write to monitor.yuv
    fwrite(monitor, sizeof(uint8_t), MONITOR_SIZE * MONITOR_SIZE, file); // Write binary data
    fclose(file);
    return SUCCESS_CODE;
}

// Writes to monitor.txt the monitor data from monitor array
int writeToMonitorFile()
{
    int i;
    FILE* file;

    // Open monitor file
    file = fopen(fileNames[MONITOR_TXT_F], "w");
    if (file == NULL)
    {
        perror(OPEN_ERR);
        return FAIL_CODE;
    }

    // Write monitor array into monitor file
    for (i = 0; i < monitorCurLen; i++)
        fprintf(file, "%02X\n", monitor[i]);
    fclose(file);
    return SUCCESS_CODE;
}

// Writes disk 2D array into diskout.txt file
int writeToDiskoutFile()
{
    int sector, offset, toBreak;
    toBreak = 0;
    FILE* file;

    // Open diskout file
    file = fopen(fileNames[DISKOUT_F], "w");
    if (file == NULL)
    {
        perror(OPEN_ERR);
        return FAIL_CODE;
    }

    // Write disk 2D array into diskout file
    for (sector = 0; sector < DISK_MAX_SECTORS && !toBreak; sector++)
        for (offset = 0; offset < DISK_SECTOR_SIZE; offset++) {
            if (sector * DISK_SECTOR_SIZE + offset >= diskCurLen) {
                toBreak = 1;
                continue;
            }
            fprintf(file, "%08X\n", disk[sector][offset]);
        }

    fclose(file);
    return SUCCESS_CODE;
}

// Writes memory array in dmemout.txt file
int writeToDmemoutFile()
{
    int i;
    FILE* file;

    // Open dmemout file
    file = fopen(fileNames[DMEMOUT_F], "w");
    if (file == NULL)
    {
        perror(OPEN_ERR);
        return FAIL_CODE;
    }

    //Write memory array into dmemout file
    for (i = 0; i < memCurLen; i++)
        fprintf(file, "%08X\n", memory[i]);
    fclose(file);
    return SUCCESS_CODE;
}

// Writes cycles globar variable into file
int writeToCyclesFile()
{
    FILE* file;

    // Open cycles file
    file = fopen(fileNames[CYCLES_F], "w");
    if (file == NULL)
    {
        perror(OPEN_ERR);
        return FAIL_CODE;
    }

    // Write cycles into file
    fprintf(file, "%lu\n", cycles);

    fclose(file);
    return SUCCESS_CODE;
}

// Writes regs list in traceOut into a file
int writeToTraceFile()
{
    int i;
    regNode* p;
    FILE* file;

    // Open trace file
    file = fopen(fileNames[TRACE_F], "w");
    if (file == NULL)
    {
        perror(OPEN_ERR);
        return FAIL_CODE;
    }
    //Walk through traceOut.regs list and print in a line every node data
    p = traceOut.regs.head;
    while (p != NULL) {
        fprintf(file, "%03X %012llX ", p->pc, p->inst);
        for (i = 0; i < REGS_SIZE - 1; i++)
            fprintf(file, "%08x ", p->regs[i]);
        fprintf(file, "%08x\n", p->regs[i]);
        p = p->next;
    }
    fclose(file);
    return SUCCESS_CODE;
}

// Writes hwRegs list in traceOut into a file and while doing it, if leds or display7seg were written, Write into thier own files their data
int writeTohwRegs_Leds_display7Seg_Files()
{
    hwRegNode* hwNode;
    FILE* hwRegFile, * ledsFile, * display7SegFile;

    //Open three files: hwregtrace, leds, display7seg
    hwRegFile = fopen(fileNames[HWREGTRACE_F], "w");
    if (hwRegFile == NULL)
    {
        perror(OPEN_ERR);
        return FAIL_CODE;
    }
    ledsFile = fopen(fileNames[LEDS_F], "w");
    if (ledsFile == NULL)
    {
        perror(OPEN_ERR);
        return FAIL_CODE;
    }
    display7SegFile = fopen(fileNames[DISPLAY7SEG_F], "w");
    if (display7SegFile == NULL)
    {
        perror(OPEN_ERR);
        return FAIL_CODE;
    }

    //Walk through traceOut.regs list and print in a line every node data to hwReg file while printing to leds and display7seg files if IORegName is their register name 
    hwNode = traceOut.hwRegs.head;
    while (hwNode != NULL) {
        fprintf(hwRegFile, "%lu %s %s %08x\n", hwNode->cycle, hwNode->RorW, hwNode->IORegName, hwNode->IORegData);
        if (strcmp(hwNode->RorW, "WRITE") == 0) { // on out: WRITE command -  if written to leds or display7Seg - write to their files
            if (strcmp(hwNode->IORegName, IORegsNames[DISPLAY_7_SEG_IOR]) == 0)
                fprintf(display7SegFile, "%lu %08x\n", hwNode->cycle, hwNode->IORegData);
            else if (strcmp(hwNode->IORegName, IORegsNames[LEDS_IOR]) == 0)
                fprintf(ledsFile, "%lu %08x\n", hwNode->cycle, hwNode->IORegData);
        }
        hwNode = hwNode->next;
    }
    fclose(hwRegFile);
    fclose(ledsFile);
    fclose(display7SegFile);

    return SUCCESS_CODE;
}

// Writes regs list in traceOut into a file
int writeToRegoutFile()
{
    int i;
    FILE* file;

    // Open regout
    file = fopen(fileNames[REGOUT_F], "w");
    if (file == NULL)
    {
        perror("Failed to write data to file.\n");
        return FAIL_CODE;
    }
    // Write register values into regout file
    for (i = 3; i < REGS_SIZE; i++)
        fprintf(file, "%08X\n", Regs[i]);
    fclose(file);
    return SUCCESS_CODE;
}


// Main funcion
int main(int argc, char* argv[])
{
    // Check args FILES_AMOUNT = 15
    if (argc != FILES_AMOUNT) {
        perror("Error - Wrong number of input files");
        return FAIL_CODE;
    }

    // Load files and initiate data structures and global variables
    if (prepare(argv) == FAIL_CODE) {
        perror("Error - Initialize failed\n");
        return FAIL_CODE;
    }
    while (!toHalt && pc < INSTRUCTION_MAX_LINES)
    {
        // Parse instructions[pc], set immediates, update trace, execute instructions[pc] and
        executeInstruction();

        // Interrupt service routine
        interruptServiceRoutine();

        //Increase cycles
        cycles++;
    }

    // Finalize - write output to files and free allocated memory
    if (finalize() == FAIL_CODE) {
        perror("Error - Finilize() failed\n");
        return FAIL_CODE;
    }
    return SUCCESS_CODE;
}


//------------------- Finaliztion -----------------------

//Write required data to output files
int finalize() {
    int success = writeToRegoutFile() | writeToTraceFile() | writeTohwRegs_Leds_display7Seg_Files() | writeToDiskoutFile() |
        writeToDmemoutFile() | writeToMonitorFile() | writeBinaryToMonitorFile() | writeToCyclesFile() |

        freeTrace() | freeIrq2();
    return success;
}

// Free trace both linked lists
int freeTrace() {
    regNode* prev1, * cur1 = traceOut.regs.head;
    prev1 = cur1;
    while (cur1 != NULL)
    {
        cur1 = cur1->next;
        free(prev1);
        prev1 = cur1;
    }
    hwRegNode* prev2, * cur2 = traceOut.hwRegs.head;
    prev2 = cur2;
    while (cur2 != NULL)
    {
        cur2 = cur2->next;
        free(prev2->IORegName);
        free(prev2->RorW);
        free(prev2);
        prev2 = cur2;
    }
    return SUCCESS_CODE;
}

int freeIrq2() {
    free(irq2.irq2Cycles);
    return SUCCESS_CODE;
}