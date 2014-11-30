#pragma once

#include <vector>

#include "vhash.h"

struct procedure_calls
{
	size_t first;
	size_t length;
	enum {
		UNUSED = 0,
		USED = 1,
	} status;
};

struct program_data
{
	vhash<procedure_calls> call_graph_procedures; // : procedure name -> call_graph
	std::vector<size_t> call_graph;               // : i -> ast node index

	std::vector<size_t> procedure_list; // : i -> ast node index
};

int compile(const char* string, size_t length, std::vector<int>& program, std::vector<int>& data);
int compile_file(const char* filename, std::vector<int>& program, std::vector<int>& data);
