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

int registers[32];

#define DATA(d) (d)
#define INSTRUCTION(i, arg1, arg2) {(int)((i << (ARG1_BITS + ARG2_BITS)) | ((arg1&ARG1_MASK) << (ARG2_BITS)) | (arg2&ARG2_MASK))}

#define GET_INSTRUCTION(data) (instruction_t)((data) >> (ARG1_BITS + ARG2_BITS))
#define GET_ARG1(data) (register_t)(((data) >> (ARG2_BITS)) & ARG1_MASK)
#define GET_ARG2(data) (register_t)((data) & ARG2_MASK)

int data[] = {
	DATA(6),
	DATA(7),
};

int program[] = {
	INSTRUCTION(I_MOVE, R_2, 0),
	INSTRUCTION(I_MOVE, R_3, 1),
	INSTRUCTION(I_DATA, R_1, R_2),
	INSTRUCTION(I_DATA, R_2, R_3),
	INSTRUCTION(I_LOAD, R_3, R_1), // Load 6
	INSTRUCTION(I_LOAD, R_1, R_2), // Load 7
	INSTRUCTION(I_MULTIPLY, R_1, R_3),
	INSTRUCTION(I_DUMP, R_1, 0),
	INSTRUCTION(I_MOVE, R_2, -1),
	INSTRUCTION(I_ADD, R_1, R_2),
	INSTRUCTION(I_DIE, 0, 0), // I die!
};

int main(int argc, char** args)
{
	struct timeb initial_time_millis, final_time_millis;
	ftime(&initial_time_millis);

	//for (int i = 0; i < 10000000; i++)
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

dead:;
	}
	ftime(&final_time_millis);
	long lapsed_ms = (long)(final_time_millis.time - initial_time_millis.time) * 1000 + (final_time_millis.millitm - initial_time_millis.millitm);
	printf("Elapsed: %dms\n", lapsed_ms);

	return 0;
}
