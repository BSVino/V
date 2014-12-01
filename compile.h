#pragma once

#include <vector>

#include "vhash.h"

#include "vm.h"

struct procedure_calls
{
	size_t first;
	size_t length;
	enum {
		UNUSED = 0,
		USED = 1,
	} status;
	size_t procedure_list_index; // : program_data[procedure_list_index]
};

struct program_data
{
	// Add any stuff here, clear it in init()
	vhash<procedure_calls> call_graph_procedures; // : procedure identifier -> call_graph
	std::vector<size_t> call_graph;               // : i -> ast node index

	std::vector<size_t> procedure_list; // : i -> ast node index

	void init();
};

int compile(const char* string, size_t length, std::vector<instruction_t>& program, std::vector<int>& data);
int compile_file(const char* filename, std::vector<instruction_t>& program, std::vector<int>& data);
