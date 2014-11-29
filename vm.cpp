#include <stdio.h>
#include <stdlib.h>

#include "v.h"
#include "vm.h"

static size_t registers[REGISTERS];

static size_t stack_size = 1024 * 1024;
static char* stack = NULL;

#define GET_INSTRUCTION(data) (instruction_t)((data) >> (ARG1_BITS + ARG2_BITS))
#define GET_ARG1(data) (register_t)(((data) >> (ARG2_BITS)) & ARG1_MASK)
#define GET_ARG2(data) (register_t)((data) & ARG2_MASK)

int vm(int* program, int* data)
{
	if (!stack)
		stack = (char*)malloc(stack_size);

	registers[R_IP] = (size_t)&program[0];
	registers[R_SP] = (size_t)&stack[0];

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
			registers[arg1] = (size_t)&data[registers[arg2]];
			break;

		case I_DATALOAD:
			registers[arg1] = data[registers[arg2]];
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

		case I_PUSH:
			Unimplemented();
			Assert((size_t)registers[R_SP] < (size_t)stack + stack_size - 1);
			*(size_t*)registers[R_SP] = registers[arg1];
			registers[R_SP] += sizeof(int);
			break;

		case I_POP:
			Unimplemented();
			Assert((size_t)registers[R_SP] > (size_t)stack);
			registers[R_SP] -= sizeof(int);
			registers[arg1] = *(size_t*)registers[R_SP];
			break;

		default:
			Unimplemented();
			break;
		}

		registers[R_IP] += sizeof(int);
	}

dead:

	printf("Program exited with result: %d\n", registers[R_1]);

	return 1;
}
