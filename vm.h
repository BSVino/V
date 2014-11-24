#pragma once

#define INSTRUCTION_BITS 10
#define ARG1_BITS 4
#define ARG2_BITS 4
#define ARG1_MASK 0xF
#define ARG2_MASK 0xF

#define DATA(d) (d)
#define INSTRUCTION(i, arg1, arg2) {(int)((i << (ARG1_BITS + ARG2_BITS)) | ((arg1&ARG1_MASK) << (ARG2_BITS)) | (arg2&ARG2_MASK))}

typedef enum {
	I_DIE = 0,  // End the program
	I_JUMP,     // R_IP += arg1
	I_MOVE,     // arg1 <- arg2
	I_LOAD,     // arg1 <- *arg2
	I_DATA,     // arg1 <- &data[arg2]
	I_ADD,      // arg1 <- arg1 + arg2
	I_MULTIPLY, // arg1 <- arg1 * arg2
	I_DUMP,     // Dump register arg1
} instruction_t;

typedef enum {
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

int vm(int* program, int* data);
