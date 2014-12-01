#pragma once

#define INSTRUCTION_BITS 10
#define ARG1_BITS 4
#define ARG2_BITS 4
#define ARG1_MASK 0xF
#define ARG2_MASK 0xF

#define DATA(d) (d)
#define INSTRUCTION(i, arg1, arg2) {(instruction_t)((i << (ARG1_BITS + ARG2_BITS)) | ((arg1&ARG1_MASK) << (ARG2_BITS)) | (arg2&ARG2_MASK))}

#define REGISTERS 16

typedef enum {
	I_DIE = 0,  // End the program
	I_JUMP,     // R_IP += arg1 -- arg1 is a constant.
	I_MOVE,     // arg1 <- arg2 -- arg2 is a constant.
	I_LOAD,     // arg1 <- *arg2
	I_DATA,     // arg1 <- &data[arg2]
	I_DATALOAD, // arg1 <- data[arg2]
	I_ADD,      // arg1 <- arg1 + arg2
	I_SUBTRACT, // arg1 <- arg1 - arg2
	I_MULTIPLY, // arg1 <- arg1 * arg2
	I_DIVIDE,   // arg1 <- arg1 / arg2
	I_DUMP,     // Dump register arg1
	I_PUSH,     // *R_SP <- arg1; R_SP++
	I_POP,      // R_SP--; arg1 <- *R_SP
	I_CALL,     // push R_IP; R_IP <- arg1
	I_RETURN,   // pop R_IP
} opcode_t;

typedef unsigned int instruction_t;

typedef enum {
	R_NONE = -1,
	R_IP = 0,
	R_SP,
	R_BP,
	R_FL,
	R_1,
	R_2,
	R_3,
	R_4,
	R_5,
	R_6,
	R_7,
	R_8,
	R_9,
	R_10,
	R_11,
	R_12,
} register_t;

int vm(instruction_t* program, int* data);

void print_instruction(instruction_t i);
