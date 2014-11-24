#pragma once

#include <vector>

typedef size_t st_string;

st_string st_add(std::vector<char>& st, const char* string, size_t length);
const char* st_get(std::vector<char>& st, st_string string);
