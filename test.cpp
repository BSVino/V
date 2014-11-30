#ifdef _WIN32
#include <sys/timeb.h>
#endif

#include <map>

#include "v.h"
#include "vhash.h"

#include "compile.h"
#include "vm.h"

using namespace std;

vector<int> program;
vector<int> data;

void test(const char* string, bool should_compile, int desired_result = 0)
{
	size_t length = strlen(string);
	int result = compile(string, length, program, data);

	if (!should_compile)
	{
		Assert(!result);
		return;
	}

	Assert(result && vm(program.data(), data.data()) == desired_result);
}

extern void test_vhash();

void do_tests()
{
	test("main ", false);
	test("main := ", false);
	test("main := ()", false);
	test("main := {}", false);
	test("main := () { return 42; }", true, 42);
	test("main := () { return 5 + 7; }", true, 12);
	test("main := () { x: int = 5 + 7; return x; }", true, 12);
	test("main := () { return 1 - 2 + 3; }", true, 2);
	test("main := () { return 1 + 2 - 3; }", true, 0);
	test("main := () { return 1 + 2 * 3; }", true, 7);
	test("main := () { x: int; return 42; }", true, 42);
	test("main := () { x:= 42; return x; }", true, 42);
	test("main := () { x: int = 42; return x; }", true, 42);
	test("main := () { x: int; x = 42; return x; }", true, 42);

	// TO CHECK:
	// * Variables not used before they are declared
	// * Assignment to constants is a compiler error
	// * Something that screws up the register order and SSA has to fix
	// * Main exists, main is a procedure, main has either no or the standard parameters

	test_vhash();
}

void test_vhash()
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
	for (int i = 0; i < 100000; i++)
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
	//printf("Elapsed: %dms\n", lapsed_ms);

	vhash<unsigned short> test2;
	test2.init(2000);

	char key_names[100][100];
	for (int i = 0; i < 100; i++)
		sprintf(key_names[i], "keytest%d", i);

	srand(0);
	ftime(&initial_time_millis);

	for (int i = 0; i < 100000; i++)
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
	//printf("Elapsed: %dms\n", lapsed_ms);

	for (int i = 0; i < 100; i++)
	{
		bool found;
		size_t index = test2.find(key_names[i], 0, &found);
		if (found)
			Assert(test1[i] == *test2.get(index));
	}
}
