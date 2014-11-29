#include "optimize.h"

#include <vector>

#include "v.h"
#include "emit.h"

#define TOP (~0 - 1) // Register is not a constant
#define BOTTOM ~0    // Register is uninitialized

using namespace std;

void optimize_copy_propagation(vector<instruction_3ac>* input, vector<register_timeline>* timeline)
{
	vector<size_t> register_constants; // Tracks whether a register holds a constant value
	register_constants.resize(input->size() + 1);

	memset(register_constants.data(), BOTTOM, sizeof(size_t) * register_constants.size());

	vector<size_t> instruction_reads; // Maps an instruction to the registers that it reads
	instruction_reads.resize(2 * (input->size() + 1));

	memset(instruction_reads.data(), ~0, sizeof(size_t) * instruction_reads.size());

	timeline->resize(input->size() + 1);
	memset(timeline->data(), ~0, sizeof(register_timeline) * timeline->size());

	vector<size_t> instructions_used; // Stack containing used instructions

	for (size_t i = 0; i < input->size(); i++)
	{
		auto& instruction = (*input)[i];

		(*input)[i].flags = (instruction_3ac::flags_e)((*input)[i].flags | instruction_3ac::I3AC_UNUSED);

		if (instruction.i == I3_DATA)
		{
			(*timeline)[instruction.r_dest].write_instruction = i;
			register_constants[instruction.r_dest] = instruction.r_arg1;
		}
		else if (instruction.i == I3_MOVE)
		{
			(*timeline)[instruction.r_dest].write_instruction = i;
			if (register_constants[instruction.r_arg1] != TOP && register_constants[instruction.r_arg1] != BOTTOM)
			{
				instruction.i = I3_DATA;
				instruction.r_arg1 = register_constants[instruction.r_arg1];
				register_constants[instruction.r_dest] = instruction.r_arg1;
			}
			else
			{
				register_constants[instruction.r_dest] = TOP;
				instruction_reads[i * 2] = instruction.r_arg1;
				(*timeline)[instruction.r_arg1].last_read = i;
			}
		}
		else if (instruction.i == I3_JUMP)
		{
			if (instruction.r_arg1 == EMIT_JUMP_END_OF_PROCEDURE)
			{
				instruction_reads[i * 2] = EMIT_RETURN_REGISTER_INDEX;
				(*timeline)[EMIT_RETURN_REGISTER_INDEX].last_read = i;
				instructions_used.push_back(i);
			}
			else
				Unimplemented();
		}
		else
			Unimplemented();
	}

	while (instructions_used.size())
	{
		size_t current_i = instructions_used.back();
		instructions_used.pop_back();

		if (!((*input)[current_i].flags & instruction_3ac::I3AC_UNUSED))
			continue;

		(*input)[current_i].flags = (instruction_3ac::flags_e)((*input)[current_i].flags & ~instruction_3ac::I3AC_UNUSED);

		size_t reg1 = instruction_reads[current_i * 2];
		if (reg1 != ~0)
			instructions_used.push_back((*timeline)[reg1].write_instruction);

		size_t reg2 = instruction_reads[current_i * 2+1];
		if (reg2 != ~0)
			instructions_used.push_back((*timeline)[reg2].write_instruction);
	}
}
