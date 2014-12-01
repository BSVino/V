#include <stdio.h>

#include <vector>

#include "vm.h"
#include "compile.h"

using namespace std;

#include <map>
#include "vhash.h"

void do_tests();

#define V_TEST

int main(int argc, char** args)
{
#ifdef V_TEST
	do_tests();
#endif

	vector<instruction_t> program;
	vector<int> data;

	if (!compile_file(args[1], program, data))
		return -1;

	vm(program.data(), data.data());

	return 0;
}

