#include "stringtable.h"

using namespace std;

st_string st_add(std::vector<char>& st, const char* string, size_t length)
{
	st_string r = st.size();
	st.resize(r + length + 1);
	strncpy(&st[r], string, length);

	return r;
}

const char* st_get(std::vector<char>& st, st_string string)
{
	return &st.data()[string];
}
