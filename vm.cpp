#include <stdio.h>

#ifdef _WIN32
#include <sys/timeb.h>
#endif

#define Assert(x) do {if (!(x)) __debugbreak(); } while (0)

#define INSTRUCTION_BITS 10
#define ARG1_BITS 4
#define ARG2_BITS 4
#define ARG1_MASK 0xF
#define ARG2_MASK 0xF

typedef enum {
	I_DIE = 0,
	I_JUMP,
	I_MOVE,
	I_LOAD,
	I_MULTIPLY,
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
#define INSTRUCTION(i, arg1, arg2) {(int)((i << (ARG1_BITS + ARG2_BITS)) | (arg1 << (ARG2_BITS)) | (arg2))}

#define GET_INSTRUCTION(data) (instruction_t)((data) >> (ARG1_BITS + ARG2_BITS))
#define GET_ARG1(data) (register_t)(((data) >> (ARG2_BITS)) & ARG1_MASK)
#define GET_ARG2(data) (register_t)((data) & ARG2_MASK)

int program[] = {
	INSTRUCTION(I_JUMP, 2, 0),
	DATA(6),
	DATA(7),
	INSTRUCTION(I_MOVE, R_2, 1),
	INSTRUCTION(I_MOVE, R_3, 2),
	INSTRUCTION(I_LOAD, R_1, R_2), // Load 6
	INSTRUCTION(I_LOAD, R_2, R_3), // Load 7
	INSTRUCTION(I_MULTIPLY, R_1, R_2),
	INSTRUCTION(I_PRINT, R_1, 0),
	INSTRUCTION(I_DIE, 0, 0), // I die!
};

int main(int argc, char** args)
{
	struct timeb initial_time_millis, final_time_millis;
	ftime(&initial_time_millis);

	//for (int i = 0; i < 10000000; i++)
	{
	registers[R_IP] = 0;

	while (true)
	{
		int current_i = program[registers[R_IP]];
		instruction_t i = GET_INSTRUCTION(current_i);
		register_t arg1 = GET_ARG1(current_i);
		register_t arg2 = GET_ARG2(current_i);
		switch (i)
		{
		case I_DIE:
			goto dead;

		case I_JUMP:
			registers[R_IP] += arg1;
			break;

		case I_MOVE:
			registers[arg1] = arg2;
			break;

		case I_LOAD:
			// TODO: Fault if arg2 invalid
			registers[arg1] = program[registers[arg2]];
			break;

		case I_MULTIPLY:
			registers[arg1] = registers[arg1] * registers[arg2];
			break;

		case I_PRINT:
			printf("Register %d = %d\n", arg1, registers[arg1]);
			break;
		}

		registers[R_IP]++;
	}

dead:;
	}
	ftime(&final_time_millis);
	long lapsed_ms = (long)(final_time_millis.time - initial_time_millis.time) * 1000 + (final_time_millis.millitm - initial_time_millis.millitm);
	printf("Elapsed: %dms\n", lapsed_ms);

	return 0;
}
