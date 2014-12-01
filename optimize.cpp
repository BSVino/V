#include "optimize.h"

#include <vector>

#include "v.h"
#include "emit.h"

struct register_info
{
	enum {
		CONSTANT =      0,
		NOT_CONSTANT =  1, // AKA top
		UNINITIALIZED = 2, // AKA bottom
	} state;
	size_t value;
};

using namespace std;

void optimize_copy_propagation(vector<instruction_3ac>* input, vector<register_timeline>* timeline)
{
	vector<register_info> register_constants; // Tracks register constant values
	register_constants.resize(input->size() + 1);

	for (size_t i = 0; i < register_constants.size(); i++)
		register_constants[i].state = register_info::UNINITIALIZED;

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

		switch (instruction.i)
		{
		case I3_DATA:
			(*timeline)[instruction.r_dest].write_instruction = i;
			register_constants[instruction.r_dest].value = instruction.r_arg1;
			register_constants[instruction.r_dest].state = register_info::CONSTANT;
			break;

		case I3_MOVE:
			(*timeline)[instruction.r_dest].write_instruction = i;
			if (!(register_constants[instruction.r_arg1].state & register_info::NOT_CONSTANT) && !(register_constants[instruction.r_arg1].state & register_info::UNINITIALIZED))
			{
				instruction.i = I3_DATA;
				instruction.r_arg1 = register_constants[instruction.r_arg1].value;
				register_constants[instruction.r_dest].value = instruction.r_arg1;
				register_constants[instruction.r_dest].state = register_info::CONSTANT;
			}
			else
			{
				register_constants[instruction.r_dest].state = register_info::NOT_CONSTANT;
				instruction_reads[i * 2] = instruction.r_arg1;
				(*timeline)[instruction.r_arg1].last_read = i;
			}
			break;

		case I3_ADD:
		case I3_SUBTRACT:
		case I3_MULTIPLY:
		case I3_DIVIDE:
			(*timeline)[instruction.r_dest].write_instruction = i;
			if (register_constants[instruction.r_arg1].state != register_info::NOT_CONSTANT && register_constants[instruction.r_arg1].state != register_info::UNINITIALIZED
				&& register_constants[instruction.r_arg2].state != register_info::NOT_CONSTANT && register_constants[instruction.r_arg2].state != register_info::UNINITIALIZED)
			{
				switch (instruction.i)
				{
				case I3_ADD:      instruction.r_arg1 = register_constants[instruction.r_arg1].value + register_constants[instruction.r_arg2].value; break;
				case I3_SUBTRACT: instruction.r_arg1 = register_constants[instruction.r_arg1].value - register_constants[instruction.r_arg2].value; break;
				case I3_MULTIPLY: instruction.r_arg1 = register_constants[instruction.r_arg1].value * register_constants[instruction.r_arg2].value; break;
				case I3_DIVIDE:   instruction.r_arg1 = register_constants[instruction.r_arg1].value / register_constants[instruction.r_arg2].value; break;
				default:
					Unimplemented();
					break;
				}
				instruction.i = I3_DATA;
				register_constants[instruction.r_dest].value = instruction.r_arg1;
				register_constants[instruction.r_dest].state = register_info::CONSTANT;
			}
			else
			{
				register_constants[instruction.r_dest].state = register_info::NOT_CONSTANT;
				instruction_reads[i * 2] = instruction.r_arg1;
				instruction_reads[i * 2 + 1] = instruction.r_arg2;
				(*timeline)[instruction.r_arg1].last_read = i;
				(*timeline)[instruction.r_arg2].last_read = i;
			}
			break;

		case I3_CALL:
			Unimplemented();
			break;

		case I3_RETURN:
			instruction_reads[i * 2] = instruction.r_arg1;
			(*timeline)[instruction.r_arg1].last_read = i;
			instructions_used.push_back(i);
			break;

		default:
			Unimplemented();
			break;
		}
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
