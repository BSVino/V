#pragma once

#include <vector>

int compile(const char* string, size_t length, std::vector<int>& program, std::vector<int>& data);
int compile_file(const char* filename, std::vector<int>& program, std::vector<int>& data);
