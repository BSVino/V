#include <sys/timeb.h>

#include <map>
#include <string>

#include "v.h"
#include "vhash.h"

#include "compile.h"
#include "vm.h"

using namespace std;

void test(const char* string, bool should_compile, int desired_result = 0) {
	vector<instruction_t> program;
	vector<int> data;

	size_t length = strlen(string);
	int compiled = compile(string, length, program, data);

	if (!should_compile) {
		Assert(!compiled);
		return;
	}

	instruction_t* program_data = program.data();
	int* data_data = data.data();

	int result = vm(program_data, data_data);
	Assert(compiled && result == desired_result);

	if (!compiled)
		printf("Compile failed.\n");
	else if (result != desired_result) {
		printf("Program listing:\n\n");
		for (size_t i = 0; i < program.size(); i++)
			print_instruction(program[i]);
	}
}

extern void test_vhash();

int main() {
	test("", false);
	test("main ", false);
	test("main := ", false);
	test("main := ()", false);
	test("main := {}", false);
	test("main := (argc: int) { return 42; }", true, 42);
	test("main := () { return 42 }", false); // Missing semicolon
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
	test("test := () { return 42; } main := () { return test(); }", true, 42);
	test("test := () { return 42; } test2 := () { return 24; } main := () { return test() + test2(); }", true, 66);
	test("test := () { return test2() + 1; } test2 := () { return 41; } main := () { return test(); }", true, 42); // Procedures out of order

	// TO CHECK:
	// * Variables not used before they are declared
	// * Assignment to constants is a compiler error
	// * Something that screws up the register order and SSA has to fix
	// * Main exists, main is a procedure, main has either no or the standard parameters
	// * Left side of assigns are l values

	test_vhash();
}

void test_vhash() {
	srand(0);

	std::string key_names_s[100];
	for (int i = 0; i < 100; i++) {
		char buf[100];
		sprintf(buf, "keytest%d", i);
		key_names_s[i] = buf;
	}

	struct timeb initial_time_millis, final_time_millis;
	ftime(&initial_time_millis);

	std::map<size_t, unsigned short> test1;
	for (int i = 0; i < 100000; i++) {
		size_t key = rand() % 100;

		const auto& it = test1.find(key);
		if (it == test1.end()) {
			test1[key] = 42;
		} else {
			it->second++;
		}
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

	for (int i = 0; i < 100000; i++) {
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

	for (int i = 0; i < 100; i++) {
		bool found;
		size_t index = test2.find(key_names[i], 0, &found);
		if (found)
			Assert(test1[i] == *test2.get(index));
	}
}
