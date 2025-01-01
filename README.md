# SIMP-Assembler-Simulator

## Description
This repository contains a university course project. The project involves developing an assembler and a simulator for the SIMP processor, a RISC-based processor similar to MIPS but with simplified functionality. The repository also includes assembly programs designed for testing the functionality of the assembler and simulator.

### SIMP Processor Description
The SIMP processor has the following features:
- **16 Registers**: Includes general-purpose registers, special-purpose registers (e.g., `$zero`, `$imm1`, `$imm2`), and control registers.
- **Instruction Set**: Supports arithmetic, logical, branch, memory, and I/O operations. Each instruction is 48 bits wide.
- **Memory**:
  - Instruction memory: 4096 lines (48 bits each).
  - Data memory: 4096 lines (32 bits each).
- **I/O Devices**:
  - LEDs
  - 7-segment display
  - 256x256 monochromatic monitor
  - Disk drive (64 KB capacity with 128 sectors)
- **Interrupts**: Supports three interrupts (irq0, irq1, irq2) with specific hardware behavior.

## Usage
The project needs to run on Windows.

### Assembler
The assembler converts assembly programs into machine code for the SIMP processor. It takes an assembly source file as input:
```Usage
asm.exe <program.asm> <imemin.txt> <dmemin.txt>
```

### Simulator
The simulator simulates the SIMP processor’s fetch-decode-execute cycle along with I/O device interactions. It reads input files, simulates the execution of the program, and generates output files detailing the state of the processor and memory.

**Input Files**
- `imemin.txt`: Instruction memory contents.
- `dmemin.txt`: Data memory contents.
- `diskin.txt`: Disk contents.
- `irq2in.txt`: External interrupt timings.

```Usage
sim.exe <imemin.txt> <dmemin.txt> <diskin.txt> <irq2in.txt> <dmemout.txt> <regout.txt> <trace.txt> <hwregtrace.txt> <cycles.txt> <leds.txt> <display7seg.txt> <diskout.txt> <monitor.txt> <monitor.yuv>
```

## Output Files
### Assembler
The assembler generates two output files:
- `imemin.txt`: Initial contents of the instruction memory.
- `dmemin.txt`: Initial contents of the data memory.

### Simulator
The simulator generates output files detailing the state of the processor and memory:
- `dmemout.txt`: Final data memory state.
- `regout.txt`: Register values.
- `trace.txt`: Execution trace of instructions.
- `hwregtrace.txt`: Hardware register access trace.
- `cycles.txt`: Total clock cycles used.
- `leds.txt`: LED status changes.
- `display7seg.txt`: 7-segment display status.
- `diskout.txt`: Final disk state.
- `monitor.txt`: Final monitor pixel values.
- `monitor.yuv`: Binary monitor pixel data.

## Testers
The repository includes four assembly test programs:
1. `tests/mulmat/mulmat.asm`: Multiplies two 4x4 matrices.
2. `tests/binom/binom.asm`: Calculates the Newton binomial coefficient recursively.
3. `test/circle/circle.asm`: Draws a circle on a 256x256 monochrome screen.
4. `test/disktest/disktest.asm`: Shifts the first 8 sectors of a disk drive forward by one sector.

Each test program is provided with its corresponding input and output files for verification.
