#include <vector>

#include "v.h"
#include "emit.h"

#define TOP (~0 - 1) // Register is not a constant
#define BOTTOM ~0    // Register is uninitialized

using namespace std;

void optimize_copy_propagation(vector<instruction_3ac>& input)
{
	vector<size_t> register_constants;
	register_constants.resize(input.size() + 1);

#ifdef _DEBUG
	memset(register_constants.data(), ~0, sizeof(size_t) * register_constants.size());
#endif

	for (size_t i = 0; i < input.size(); i++)
	{
		auto& instruction = input[i];

		if (instruction.i == I3_DATA)
			register_constants[instruction.r_dest] = instruction.r_arg1;
		else if (instruction.i == I3_MOVE)
		{
			if (register_constants[instruction.r_arg1] != TOP && register_constants[instruction.r_arg1] != BOTTOM)
			{
				instruction.i = I3_DATA;
				instruction.r_arg1 = register_constants[instruction.r_arg1];
				register_constants[instruction.r_dest] = instruction.r_arg1;
			}
			else
				register_constants[instruction.r_dest] = TOP;
		}
		else if (instruction.i == I3_JUMP)
		{
			if (instruction.r_arg1 != EMIT_JUMP_END_OF_PROCEDURE)
				Unimplemented();
		}
		else
			Unimplemented();
	}
}
