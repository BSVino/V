#include <stdio.h>

#include <vector>

#include "vm.h"
#include "compile.h"

#ifdef _WIN32
#include <sys/timeb.h>
#endif

using namespace std;

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
	vector<int> program_c;
	vector<int> data_c;

	if (!compile(args[1], program_c, data_c))
		return -1;

	struct timeb initial_time_millis, final_time_millis;
	ftime(&initial_time_millis);

	//for (int i = 0; i < 10000000; i++)
	{
		vm(program, data);
	}

	ftime(&final_time_millis);
	long lapsed_ms = (long)(final_time_millis.time - initial_time_millis.time) * 1000 + (final_time_millis.millitm - initial_time_millis.millitm);
	printf("Elapsed: %dms\n", lapsed_ms);

	return 0;
}
