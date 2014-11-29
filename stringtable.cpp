#include "stringtable.h"

using namespace std;

st_string st_add(std::vector<char>& st, const char* string, size_t length)
{
	// First a linear search to see if the symbol is already here.
	// This could probably be accelerated somehow.
	// We need this so that identical strings have identical addresses,
	// some algorithms require that (ie any algorithm that uses map<const char*, x>)
	if (st.size())
	{
		char* s = &st[0];
		size_t size = st.size();
		char* end = s + size;
		while (s < end)
		{
			if (strncmp(s, string, length) == 0)
				return (st_string)(s - &st[0]);
			s += strlen(s) + 1;
		}
	}

	st_string r = st.size();
	st.resize(r + length + 1);
	strncpy(&st[r], string, length);

	return r;
}

const char* st_get(std::vector<char>& st, st_string string)
{
	return &st.data()[string];
}
