#include <stdio.h>

#include "v.h"
#include "vm.h"

int registers[32];

#define GET_INSTRUCTION(data) (instruction_t)((data) >> (ARG1_BITS + ARG2_BITS))
#define GET_ARG1(data) (register_t)(((data) >> (ARG2_BITS)) & ARG1_MASK)
#define GET_ARG2(data) (register_t)((data) & ARG2_MASK)

int vm(int* program, int* data)
{
	registers[R_IP] = (int)&program[0];

	while (true)
	{
		int current_i = *(int*)registers[R_IP];
		instruction_t i = GET_INSTRUCTION(current_i);
		register_t arg1 = GET_ARG1(current_i);
		register_t arg2 = GET_ARG2(current_i);
		switch (i)
		{
		case I_DIE:
			goto dead;

		case I_JUMP:
			registers[R_IP] += arg1 * sizeof(int);
			break;

		case I_MOVE:
			registers[arg1] = (char)(arg2 << 4) >> 4; // Abuse signed shifting to propogate negatives
			break;

		case I_LOAD:
			registers[arg1] = *(int*)registers[arg2];
			break;

		case I_DATA:
			registers[arg1] = (int)&data[registers[arg2]];
			break;

		case I_ADD:
			registers[arg1] = registers[arg1] + registers[arg2];
			break;

		case I_MULTIPLY:
			registers[arg1] = registers[arg1] * registers[arg2];
			break;

		case I_DUMP:
			printf("Register %d = %d\n", arg1, registers[arg1]);
			break;
		}

		registers[R_IP] += sizeof(int);
	}

dead:

	return 0;
}
