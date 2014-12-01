#pragma once

#define INSTRUCTION_BITS 32
#define OPCODE_BITS 10
#define ARG1_BITS 4
#define ARG2_BITS 4
#define ARG1_MASK 0xF
#define ARG2_MASK 0xF
#define CALL_ARG_BITS 22
#define CALL_ARG_MASK 0x3FFFFF

#define DATA(d) (d)
#define INSTRUCTION(i, arg1, arg2) {(instruction_t)((i << (INSTRUCTION_BITS - OPCODE_BITS)) | ((arg1&ARG1_MASK) << (ARG2_BITS)) | (arg2&ARG2_MASK))}
#define INSTRUCTION_CALL(arg) {(instruction_t)((I_CALL << (INSTRUCTION_BITS - OPCODE_BITS)) | (arg&CALL_ARG_MASK))}

#define GET_OPCODE(data) (opcode_t)((data) >> (INSTRUCTION_BITS - OPCODE_BITS))
#define GET_ARG1(data) (register_t)(((data) >> (ARG2_BITS)) & ARG1_MASK)
#define GET_ARG2(data) (register_t)((data) & ARG2_MASK)
#define GET_CALL_ARG(data) (size_t)((data) & CALL_ARG_MASK)

#define REGISTERS 16

typedef enum {
	I_DIE = 0,  // End the program
	I_JUMP,     // R_IP += arg1 -- arg1 a constant
	I_MOVE,     // arg1 <- arg2 -- arg1 a register, arg2 a constant
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
	I_CALL,     // push R_IP; R_IP <- R_IP + arg1 -- arg1 a constant
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
