#include <stdio.h>
#include <stdlib.h>

#include "v.h"
#include "vm.h"

static size_t registers[REGISTERS];

static size_t stack_size = 1024 * 1024;
static char* stack = NULL;

int vm(instruction_t* program, int* data)
{
	if (!stack)
		stack = (char*)malloc(stack_size);

	registers[R_IP] = (size_t)&program[0];
	registers[R_SP] = (size_t)&stack[0];

	while (true)
	{
		instruction_t current_i = *(instruction_t*)registers[R_IP];
		opcode_t i = GET_OPCODE(current_i);
		register_t arg1 = GET_ARG1(current_i);
		register_t arg2 = GET_ARG2(current_i);
		switch (i)
		{
		case I_DIE:
			goto dead;

		case I_JUMP:
			registers[R_IP] += arg1 * sizeof(instruction_t);
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

		case I_SUBTRACT:
			registers[arg1] = registers[arg1] - registers[arg2];
			break;

		case I_MULTIPLY:
			registers[arg1] = registers[arg1] * registers[arg2];
			break;

		case I_DIVIDE:
			registers[arg1] = registers[arg1] / registers[arg2];
			break;

		case I_DUMP:
			printf("Register %d = %d\n", arg1, registers[arg1]);
			break;

		case I_PUSH:
			Unimplemented();
			Assert((size_t)registers[R_SP] < (size_t)stack + stack_size - 1);
			*(size_t*)registers[R_SP] = registers[arg1];
			registers[R_SP] += sizeof(size_t);
			break;

		case I_POP:
			Unimplemented();
			Assert((size_t)registers[R_SP] > (size_t)stack);
			registers[R_SP] -= sizeof(size_t);
			registers[arg1] = *(size_t*)registers[R_SP];
			break;

		case I_CALL:
		{
			size_t call_arg = GET_CALL_ARG(current_i);
			Assert((size_t)registers[R_SP] < (size_t)stack + stack_size - 1);
			*(size_t*)registers[R_SP] = registers[R_IP];
			registers[R_SP] += sizeof(size_t);
			registers[R_IP] += call_arg * sizeof(instruction_t);
			break;
		}

		case I_RETURN:
			Assert((size_t)registers[R_SP] > (size_t)stack);
			registers[R_SP] -= sizeof(size_t);
			registers[R_IP] = *(size_t*)registers[R_SP];
			break;

		default:
			Unimplemented();
			break;
		}

		registers[R_IP] += sizeof(instruction_t);
	}

dead:

	printf("Program exited with result: %d\n", registers[R_1]);

	return registers[R_1];
}

const char* print_register(register_t r)
{
	switch (r)
	{
	case R_NONE: return "R_NONE";
	case R_IP: return "R_IP";
	case R_SP: return "R_SP";
	case R_BP: return "R_BP";
	case R_FL: return "R_FL";
	case R_1:  return "R_1";
	case R_2:  return "R_2";
	case R_3:  return "R_3";
	case R_4:  return "R_4";
	case R_5:  return "R_5";
	case R_6:  return "R_6";
	case R_7:  return "R_7";
	case R_8:  return "R_8";
	case R_9:  return "R_9";
	case R_10: return "R_10";
	case R_11: return "R_11";
	case R_12: return "R_12";

	default:
		Unimplemented();
		return "R_UNKNOWN";
	}
}

void print_instruction(instruction_t print_i)
{
	opcode_t i = GET_OPCODE(print_i);
	register_t arg1 = GET_ARG1(print_i);
	register_t arg2 = GET_ARG2(print_i);

	switch (i)
	{
	case I_DIE:      printf("I_DIE\n"); break;
	case I_JUMP:     printf("I_JUMP %d\n", arg1); break;
	case I_MOVE:     printf("I_MOVE %s %d\n", print_register(arg1), arg2); break;
	case I_LOAD:     printf("I_LOAD %s %s\n", print_register(arg1), print_register(arg2)); break;
	case I_DATA:     printf("I_DATA %s %s\n", print_register(arg1), print_register(arg2)); break;
	case I_DATALOAD: printf("I_DATALOAD %s %s\n", print_register(arg1), print_register(arg2)); break;
	case I_ADD:      printf("I_ADD %s %s\n", print_register(arg1), print_register(arg2)); break;
	case I_SUBTRACT: printf("I_SUBTRACT %s %s\n", print_register(arg1), print_register(arg2)); break;
	case I_MULTIPLY: printf("I_MULTIPLY %s %s\n", print_register(arg1), print_register(arg2)); break;
	case I_DIVIDE:   printf("I_DIVIDE %s %s\n", print_register(arg1), print_register(arg2)); break;
	case I_DUMP:     printf("I_DUMP %s\n", print_register(arg1)); break;
	case I_PUSH:     printf("I_PUSH %s\n", print_register(arg1)); break;
	case I_POP:      printf("I_POP %s\n", print_register(arg1)); break;
	case I_CALL:     printf("I_CALL %d\n", GET_CALL_ARG(print_i)); break;
	case I_RETURN:   printf("I_RETURN\n"); break;

	default:
		Unimplemented();
		printf("Unknown instruction.\n");
		break;
	}
}
