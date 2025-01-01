/*
	@brief Classify the string between decimal, hexadecimal or word represntation
	@param imm - The string to classify
	@retval 1 - If imm represent decimal number
	@retval 2 - If imm represent hexadecimal number
	@retval 3 - If imm represent a word
*/
int classifyImmediate(char* imm)
{
	char* endptr;
	int i;

	if (isalpha(imm[0]) != 0) { // Starts with a letter -> it's a label
		return 3;
	}

	if (imm[0] == '0' && (imm[1] == 'x' || imm[1] == 'X')) { // Starts with 0x or 0X -> it's a hexadecimal
		return 2;
	}

	// Otherwise -> it's a decimal
	return 1;
}