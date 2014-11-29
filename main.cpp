#include <stdio.h>

#include <vector>

#include "vm.h"
#include "compile.h"

#ifdef _WIN32
#include <sys/timeb.h>
#endif

using namespace std;

int data[] = {
	DATA(6),
	DATA(7),
};

int program[] = {
	INSTRUCTION(I_MOVE, R_2, 0),
	INSTRUCTION(I_MOVE, R_3, 1),
	INSTRUCTION(I_DATA, R_1, R_2),
	INSTRUCTION(I_DATA, R_2, R_3),
	INSTRUCTION(I_LOAD, R_3, R_1), // Load 6
	INSTRUCTION(I_LOAD, R_1, R_2), // Load 7
	INSTRUCTION(I_MULTIPLY, R_1, R_3),
	INSTRUCTION(I_DUMP, R_1, 0),
	INSTRUCTION(I_MOVE, R_2, -1),
	INSTRUCTION(I_ADD, R_1, R_2),
	INSTRUCTION(I_DIE, 0, 0), // I die!
};

#include <map>
#include "vhash.h"

void do_tests();

int main(int argc, char** args)
{
#if 0
	do_tests();
	return 0;
#endif

	vector<int> program_c;
	vector<int> data_c;

	if (!compile(args[1], program_c, data_c))
		return -1;

	vm(program_c.data(), data_c.data());

	return 0;
}

void do_tests()
{
	srand(0);

	std::string key_names_s[100];
	for (int i = 0; i < 100; i++)
	{
		char buf[100];
		sprintf(buf, "keytest%d", i);
		key_names_s[i] = buf;
	}

	struct timeb initial_time_millis, final_time_millis;
	ftime(&initial_time_millis);

	std::map<size_t, unsigned short> test1;
	for (int i = 0; i < 10000000; i++)
	{
		size_t key = rand() % 100;

		auto& it = test1.find(key);
		if (it == test1.end())
			test1[key] = 42;
		else
			it->second++;
	}

	ftime(&final_time_millis);
	long lapsed_ms = (long)(final_time_millis.time - initial_time_millis.time) * 1000 + (final_time_millis.millitm - initial_time_millis.millitm);
	printf("Elapsed: %dms\n", lapsed_ms);

	vhash<unsigned char> test2;
	test2.init(2000);

	char key_names[100][100];
	for (int i = 0; i < 100; i++)
		sprintf(key_names[i], "keytest%d", i);

	srand(0);
	ftime(&initial_time_millis);

	for (int i = 0; i < 10000000; i++)
	{
		size_t key = rand() % 100;
		const char* key_name = key_names[key];

		vhash<unsigned char>::hash_t key_hash;
		bool found;
		auto it = test2.find(key_name, &key_hash, &found);
		if (found)
			(*test2.get(it))++;
		else
			test2.set(key_name, it, key_hash, 42);
	}

	ftime(&final_time_millis);
	lapsed_ms = (long)(final_time_millis.time - initial_time_millis.time) * 1000 + (final_time_millis.millitm - initial_time_millis.millitm);
	printf("Elapsed: %dms\n", lapsed_ms);
}
