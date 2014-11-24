#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include "v.h"
#include "lex.h"
#include "emit.h"

using namespace std;

static std::vector<int>* program;
static std::vector<int>* data;

int compile(const char* filename, std::vector<int>& input_program, std::vector<int>& input_data)
{
	FILE* fp = fopen(filename, "rb");

	fseek(fp, 0, SEEK_END);
	int file_size = ftell(fp);
	rewind(fp);

	char* file_contents = (char*)malloc(file_size+1);

	int read = fread(file_contents, 1, file_size, fp);
	file_contents[file_size] = 0;

	fclose(fp);

	lex_begin(file_contents, file_size);

	free(file_contents);

	input_program.clear();
	input_data.clear();
	program = &input_program;
	data = &input_data;

	if (ast_main == ~0)
		LEX_ERROR("No main procedure found.\n");

	if (!emit_procedure(ast_main))
		return 0;

	return 1;
}
