#include "readFile.h"

/*
	@brief Reads the assembly program file and update the instruction, data memory and word arrays
	@param filename - Assembly program filename
	@param IC - Pointer to the Instruction Counter
	@param LC - Pointer to the Label Counter
	@param WC - Pointer to the Word Counter
	@return 0 - Successfully read the assembly program file and updated the arrays
	@return 1 - Failure
*/
int ReadProgramFile(char* filename, int* IC, int* LC, int* WC)
{
	int PC = 0;
	FILE* file;
	char line_buffer[5 * MaxLabelLenth]; // will be used to read lines from the file

	file = fopen(filename, "r");

	if (file == NULL)
	{
		perror("Error opening the file");
		return 1;
	}

	// Reading lines from the file
	while (fgets(line_buffer, sizeof(line_buffer), file) != NULL)
	{

		int colon_position = search_char(line_buffer, ':'); // Searching if there is a : in the line
		int hash_position = search_char(line_buffer, '#'); // Seatching if there is a # in the line

		trimWhitespace(line_buffer);

		// Checking if it's an empty line
		if (strlen(line_buffer) <= 0) 
			continue;

		// Checking if it's a comment line
		if (line_buffer[0] == '#')
			continue;

		// Checking if it's a pseudo-instruction
		if (line_buffer[0] == '.')
		{
			int space_position = search_char(line_buffer, ' ');
			if (add_to_word_array(*WC, line_buffer + space_position + 1)) {
				fprintf(stderr, "Error parsing the instruction\n");
				return 1;
			}
			(*WC)++;
			continue;
		}

		// : is found -> there is a lable in the line
		if (colon_position != -1)
		{
			// The : are of the label, and not in a comment
			if ((hash_position != -1 && colon_position < hash_position) || hash_position == -1) {
				char label_name[MaxLabelLenth + 1];
				int label_address, next_code_line;

				line_buffer[colon_position] = '\0';

				strcpy(label_name, line_buffer);

				// Copying the part after the ':' to the beginning of line_buffer
				strcpy(line_buffer, line_buffer + colon_position + 1);
				trimWhitespace(line_buffer);


				if (strlen(line_buffer) <= 0) // The label is the only thing in the line
				{
					// Giving a special address, so we'll know to update it when we get to a code line
					if (add_to_label_array(*LC, label_name, -1)) {
						fprintf(stderr, "Error parsing the instruction\n");
						return 1;
					}
					(*LC)++;
					continue;
				}
				else // The instruction is in the same line as the label
				{
					if (add_to_label_array(*LC, label_name, PC)) { // Entring the label to the label_arr
						fprintf(stderr, "Error parsing the instruction\n");
						return 1;
					}
					(*LC)++;

					// Checking if it's a pseudo-instruction after the label
					if (line_buffer[0] == '.')
					{
						int space_position = search_char(line_buffer, ' ');
						if (add_to_word_array(*WC, line_buffer + space_position + 1)) {
							fprintf(stderr, "Error parsing the instruction\n");
							return 1;
						}
						(*WC)++;
						continue;
					}
				}
			}
		}	

		if (add_to_instruction_array(*IC, line_buffer)) {
			fprintf(stderr, "Error parsing the instruction\n");
			return 1;
		}
		update_special_labels(*LC, PC);
		(*IC)++;

		PC++;
	}

	fclose(file);
	return 0;
}

/*
	@brief Add label and label's address to the label array
	@param LC - The first free place in the label array
	@param label_name - The label name to insert to the array
	@param address - The address of the label to insert to the array
	@return 0 - Successfully added the label to the label array
	@return 1 - Faild to add the label to the label array
*/
int add_to_label_array(int LC, char* label_name, int address)
{
	strcpy(label_arr[LC].lname, label_name);
	label_arr[LC].laddr = address;
	return 0;
}

/*
	@brief Add address and data to the word array
	@param WC - The first free place in the word array
	@param instruction - The address and the data as it written in the assembly code
	@return 0 - Successfully added the word to the word array
	@return 1 - Faild to add the word to the word array
*/
int add_to_word_array(int WC, char* instruction)
{
	char address[MaxAddressLength + 1], data[MaxDataLength + 1];
	int word_position;
	char* endptr;
	int32_t dataResult = 0, addressResult = 0;


	if ((sscanf(instruction, "%s %s", address, data) != 2) && (sscanf(instruction, "%s\t%s", address, data) != 2))
	{
		fprintf(stderr, "Error parsing the .word instruction\n");
		return 1;
	}


	switch (classifyImmediate(address))
	{
		case 1:
			sscanf(address, "%d", &addressResult);
			break;
		case 2:
			sscanf(address, "%x", &addressResult);
			break;
		case 3: // address is a label name, which should not happen
			fprintf(stderr, "Error parsing the .word instruction\n");
			return 1;
	}

	word_arr[WC].address = addressResult; // Updating the word array with the new address

	switch (classifyImmediate(data))
	{
	case 1:
		sscanf(data, "%d", &dataResult);
		break;
	case 2:
		sscanf(data, "%x", &dataResult);
		break;
	case 3: // data is a label name, which should not happen
		fprintf(stderr, "Error parsing the .word instruction\n");
		return 1;
	}
	
	word_arr[WC].data = dataResult; // Updating the word array with the new data

	word_arr[WC].wasWritten = 0;

	return 0;
}

/*
	@brief Add regular instruction to the instruction array
	@param IC - The first free place in the instruction array
	@param instruction - The instruction as it written in the assembly code
	@return 0 - Successfully added the instruction to the instruction array
	@return 1 - Faild to add the instruction to the instruction array
*/
int add_to_instruction_array(int IC, char* instruction)
{
	int word_position, comma_position, hash_postion;
	int i;
	char word_buffer[2 * MaxLabelLenth]; // will be used to read words from the file

	// Reading the opcode
	if (sscanf(instruction, "%s", word_buffer) != 1) {
		fprintf(stderr, "Error parsing the instruction\n");
		return 1;
	}

	strcpy(instruc_arr[IC].opcode, word_buffer);

	// Removing the opcode from the instruction
	word_position = search_str(instruction, word_buffer); // word_position = the location of the first letter of the opcode
	if (word_position == -1) {
		fprintf(stderr, "Error parsing the instruction\n");
		return 1;
	}
	strcpy(instruction, instruction + word_position + strlen(word_buffer)); // the strlen is to remove the entire opcode

	trimWhitespace(instruction); // removing leading whitespaces if there is any

	// Reading the operands
	// There are 6 operands, separated by a comma -> There are 5 commas
	for (i = 0; i < 5; i++)
	{
		comma_position = search_char(instruction, ',');

		if (comma_position == -1)
		{
			fprintf(stderr, "Error parsing the instruction\n");
			return 1;
		}

		instruction[comma_position] = '\0';

		strcpy(instruc_arr[IC].operands[i], instruction); // Copying the operand to the instruction array

		if (i == 4) // The operand of immediate1
		{
			if (classifyImmediate(instruction) == 3) // It is a label
				instruc_arr[IC].usingLabel = 1;
			
			else
				instruc_arr[IC].usingLabel = 0;
			
		}

		strcpy(instruction, instruction + comma_position + 1); // Removing the operand from the instruction

		trimWhitespace(instruction);
	}

	// The first word in instruction now is the last operand
	hash_postion = search_char(instruction, '#');
	if (hash_postion != -1) // There is a comment in the line
	{
		// We do this because the comment can start without a space between the comment and the last operand
		instruction[hash_postion] = '\0';
	}

	trimWhitespace(instruction);

	strcpy(instruc_arr[IC].operands[i], instruction); // Copying the last operand to the instruction array
	if (classifyImmediate(instruction) == 3)          // It is a label
		instruc_arr[IC].usingLabel = 1;

	return 0;
}

/*
	@brief Trim leading and trailing whitespaces in a string
	@param str - The string to remove leading whitespaces from
*/
void trimWhitespace(char* str) {
	// Trim leading whitespace
	int index = 0;
	int len;
	while (isspace((unsigned char)str[index])) {
		index++;
	}

	// Shift the string to remove leading whitespaces
	memmove(str, str + index, strlen(str + index) + 1);

	// Trim trailing whitespace
	len = strlen(str);
	while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\n')) {
		len--;
	}

	// Copy the trimmed portion back to the beginning of the string
	memmove(str, str, len);
	str[len] = '\0';  // Null-terminate the string
}

/*
	@brief Update the label address of labels who got the special address -1 to the given PC
		   (The labels who got -1 are the ones who doesn't have a code in the same line with them)
	@param LC - The last place of added label
	@param PC - The new address of the labels
*/
void update_special_labels(int LC, int PC)
{
	LC--; // LC points to the first empty place in the label_arr
	while (label_arr[LC].lname != '\0' && label_arr[LC].laddr == -1) {
		label_arr[LC].laddr = PC;
		LC--;
	}
}

/*
	@brief Search for ch in str. If exists, return the index of ch
	@param str - The string to seach in
	@param ch - The char to search for
	@return index of ch in the str - If ch exist in str
	@return -1 - If ch doesnt exist in str
*/
int search_char(char* str, char ch)
{
	int i;

	for (i = 0; i < strlen(str); i++) {
		if (str[i] == ch) {
			return i;
		}
	}

	return -1;
}

/*
	@brief Search for s in str. If exists, return the index of the first char of s in str
	@param str - The string to seach in
	@param s - The string to search for
	@return index of the first char of s in the str - If s exist in str
	@return -1 - If s doesnt exist in str
*/
int search_str(char* str, char* s)
{
	int i;

	for (i = 0; i < strlen(str); i++) {
		if (str[i] == s[0]) {
			for (int j = 1; j < strlen(s); j++) {
				if (j > strlen(str)) { // out of str scope
					return -1;
				}
				if (str[j] != s[j]) {
					return -1;
				}
			}
			return i;
		}
	}

	return -1;
}