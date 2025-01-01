#include "updateLabels.h"

/*
	@brief Update the instruction array with the addresses of the labels, instead of their names
	@param instruc_arr_size - The size of the instruction array
	@param label_arr_size - The size of the label array
	@return 0 - On success
	@return 1 - On failure
*/
int UpdateLabels(int instruc_arr_size, int label_arr_size)
{
	int IC = 0;

	for (IC = 0; IC < instruc_arr_size; IC++) {
		if (instruc_arr[IC].usingLabel == 1) { // There is a label in the
			if (classifyImmediate(instruc_arr[IC].operands[4]) == 3) { //immediate1 is a label
				if (update_label(instruc_arr[IC].operands[4], label_arr_size)) {
					fprintf(stderr, "Error parsing the labels\n");
					return 1;
				}
			}
			if (classifyImmediate(instruc_arr[IC].operands[5]) == 3) { //immediate2 is a label
				if (update_label(instruc_arr[IC].operands[5], label_arr_size)) {
					fprintf(stderr, "Error parsing the labels\n");
					return 1;
				}
			}

		}
	}

	return 0;
}

/*
	@brief Update the operand from label name to its address
	@param operand - The operand field that need to be updated
	@param label_arr_size - The size of the label array
	@return 0 - On success
	@return 1 - On failure
*/
int update_label(char* operand, int label_arr_size)
{
	int addr = search_label(operand, label_arr_size);
	if (addr == -1) {
		fprintf(stderr, "Error parsing the labels\n");
		return 1;
	}
	sprintf(operand, "%d", addr); // Converting the addr to string
	return 0;
}

/*
	@brief Searching for label name in the label_arr, and returning it's address
	@param lname - The label to search for
	@param label_arr_size - The size of the label array
	@return laddr - The address of the label
	@return -1 - If an error occured
*/
int search_label(char* lname, int label_arr_size)
{
	for (int LC = 0; LC < label_arr_size; LC++) {
		if (strcmp(label_arr[LC].lname, lname) == 0) {
			return label_arr[LC].laddr;
		}
	}
	return -1;
}
