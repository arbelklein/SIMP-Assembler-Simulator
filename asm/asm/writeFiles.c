#include "writeFiles.h"

/*
	@brief Writes instructions and memory to the given files
	@param instruction_filename - The file name of the instructions
	@param memory_filename - The file name of the memory
	@param inst_size - The size of the instruction array
	@param mem_size - The size of the word array
	@return 0 - On success
	@return 1 - On failure
*/
int WriteToFiles(char* instruction_filename, char* memory_filename, int inst_size, int mem_size)
{
	if (write_instructions(instruction_filename, inst_size)) {
		fprintf(stderr, "Error while writing files\n");
		return 1;
	}
	if (write_memory(memory_filename, mem_size)) {
		fprintf(stderr, "Error while writing files\n");
		return 1;
	}
}

/*
	@brief Writes the instruction to a file
	@param filename - The file to write to
	@param size - The size of instruction array
	@return 0 - On success
	@return 1 - On failure
*/
int write_instructions(char* filename, int size)
{
	FILE* file;
	int j, i;
	int opcode, reg, imm;
	char* endptr;

	file = fopen(filename, "w");

	if (file == NULL) {
		perror("Error opening the file");
		return 1;
	}

	// Writing the instructions to the file
	for (i = 0; i < size; i++) {
		opcode = convert_opcode(instruc_arr[i].opcode);
		if (opcode == -1) {
			fprintf(stderr, "Error while writing files\n");
			return 1;
		}

		fprintf(file, "%02X", opcode); // Printing the opcode as 2 digit hexadecimal

		// Going over the operands (the last two operands are special (immediate)
		for (j = 0; j < (NumOfOperands - 2); j++) {
			reg = convert_register(instruc_arr[i].operands[j]);
			if (reg == -1) {
				fprintf(stderr, "Error while writing files\n");
				return 1;
			}

			fprintf(file, "%01X", reg); // Printing the register as 1 digit hexadecimal
		}

		// Going over the immediates
		for (; j < NumOfOperands; j++) {

			switch (classifyImmediate(instruc_arr[i].operands[j]))
			{
			case 1:
				sscanf(instruc_arr[i].operands[j], "%d", &imm);
				break;
			case 2:
				sscanf(instruc_arr[i].operands[j], "%x", &imm);
				break;
			case 3: // instruc_arr[i].operands[j] is a label name, which should not happen because we converted it to a number
				fprintf(stderr, "Error while writing files\n");
				return 1;
			}

			if (imm < 0) { // Negative number need special care
				imm = (unsigned int)imm & 0xFFF; // Apply bitmask to keep only 3 digits
			}

			fprintf(file, "%03X", imm); // Printing the immediate as 3 digit hexadecimal
		}

		fprintf(file, "\n");
	}

	fclose(file);
	return 0;
}

/*
	@brief Converts the opcode from str to code (int)
	@param opcode - The string version of the opcode
	@return The code (int) of the given opcode
	@return -1 - If error occurs
*/
int convert_opcode(char* opcode)
{
	for (int i = 0; i < NumOfOpcodes; i++) {
		if (strcmp(opcode, commands_names[i]) == 0) {
			return i;
		}
	}

	return -1;
}

/*
	@brief Converts the regsiter from str to code (int)
	@param opcode - The string version of the register
	@return The code (int) of the given register
	@return -1 - If error occurs
*/
int convert_register(char* reg)
{
	for (int i = 0; i < NumOfRegsiters; i++) {
		if (strcmp(reg, registers_names[i]) == 0) {
			return i;
		}
	}

	return -1;
}

/*
	@brief Writes the memory to a file
	@param - The file to write to
	@param - The size of word array
	@return 0 - On success
	@return 1 - On failure
*/
int write_memory(char* filename, int size)
{
	FILE* file;
	int line = 0;
	int32_t data;

	file = fopen(filename, "w");

	if (file == NULL) {
		perror("Error opening the file");
		return 1;
	}

	// If the word array is empty, no need to write to the file
	if (size != 0) {

		while (more_to_write(size)) {
			data = search_word(line, size);

			fprintf(file, "%08X\n", data);

			line++;
		}
	}

	fclose(file);
	return 0;
}

/*
	@brief Search in the word array for the address, and return the data of the address
	@param addr - The address to search for
	@param size - The size of the word array
	@return The data of the address
*/
int32_t search_word(int addr, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (word_arr[i].address == addr) {
			word_arr[i].wasWritten = 1;
			return word_arr[i].data;
		}
	}

	return 0;
}

/*
	@brief Check if there is more to write to memory
	@param size - The size of the word array
	@return 1 - If there is more to write
	@return 0 - If there isn't
*/
int more_to_write(int size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (word_arr[i].wasWritten == 0) {
			return 1;
		}
	}

	return 0;
}