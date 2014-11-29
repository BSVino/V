#pragma once

#include "emit.h"

struct register_timeline
{
	size_t write_instruction;
	size_t last_read;
};

extern void optimize_copy_propagation(std::vector<instruction_3ac>* input, std::vector<register_timeline>* timeline);
