#include <stdio.h>

#define Assert(x) do {if (!(x)) __debugbreak(); } while (0)

#define INSTRUCTION_BITS 10
#define ARG1_BITS 5
#define ARG2_BITS 5
#define ARG1_MASK 0xF
#define ARG2_MASK 0xF
#define P1_MASK 0x1
#define P2_MASK 0x1

typedef enum {
	I_DIE = 0,
	I_JUMP,
	I_MOVE,
	I_LOAD,
	I_PRINT,
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

int registers[32];

#define DATA(d) (d)
#define INSTRUCTION(i, p1, arg1, p2, arg2) {(int)((i << (ARG1_BITS + ARG2_BITS)) | (p1 << (ARG1_BITS + ARG2_BITS -1)) | (arg1 << (ARG2_BITS)) | (p2 << (ARG2_BITS - 1) | (arg2)))}

#define GET_INSTRUCTION(data) (instruction_t)((data) >> (ARG1_BITS + ARG2_BITS))
#define GET_P1(data) (((data) >> (ARG1_BITS + ARG2_BITS -1)) & P1_MASK)
#define GET_ARG1(data) (register_t)(((data) >> (ARG2_BITS)) & ARG1_MASK)
#define GET_P2(data) (((data) >> (ARG2_BITS - 1)) & P2_MASK)
#define GET_ARG2(data) (register_t)((data) & ARG2_MASK)

#define REG 0
#define CNS 1

int program[] = {
	INSTRUCTION(I_JUMP, CNS, 1, CNS, 0),
	DATA(42),
	INSTRUCTION(I_MOVE, REG, R_2, CNS, 1),
	INSTRUCTION(I_LOAD, REG, R_1, REG, R_2),
	INSTRUCTION(I_PRINT, REG, R_1, CNS, 0),
	INSTRUCTION(I_DIE, 0, 0, 0, 0), // I die!
};

int main(int argc, char** args)
{
	registers[R_IP] = 0;

	while (true)
	{
		int current_i = program[registers[R_IP]];
		instruction_t i = GET_INSTRUCTION(current_i);
		char p1 = GET_P1(current_i);
		char p2 = GET_P2(current_i);
		register_t arg1 = GET_ARG1(current_i);
		register_t arg2 = GET_ARG2(current_i);
		switch (i)
		{
		case I_DIE:
			return 0;

		case I_JUMP:
			registers[R_IP] += p1? arg1 : registers[arg1];
			break;

		case I_MOVE:
			registers[arg1] = p2 ? arg2 : registers[arg2];
			break;

		case I_LOAD:
			// TODO: Fault if arg2 invalid
			registers[arg1] = p2 ? program[arg2] : program[registers[arg2]];
			break;

		case I_PRINT:
			if (p1)
				printf("Constant = %d\n", arg1);
			else
				printf("Register %d = %d\n", arg1, registers[arg1]);
			break;
		}

		registers[R_IP]++;
	}

	return 0;
}
