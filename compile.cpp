#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include "v.h"
#include "lex.h"
#include "emit.h"

using namespace std;

int compile(const char* filename, std::vector<int>& program, std::vector<int>& data)
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

	if (ast_main == ~0)
		LEX_ERROR("No main procedure found.\n");

	if (!emit_begin(ast_main, program, data))
		return 0;

	return 1;
}
