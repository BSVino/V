#include "stringtable.h"

using namespace std;

st_string st_add(std::vector<char>& st, const char* string, size_t length)
{
	st_string r = st.size();
	st.resize(r + length + 1);
	strncpy(&st[r], string, length);

	return r;
}
