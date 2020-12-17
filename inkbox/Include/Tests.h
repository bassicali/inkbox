
#pragma once

#include <vector>
#include "Common.h"

#define DEFN_TEST(name) bool name(); namespace { TestRoutine name##_Instance(#name, &name); } bool name()

typedef bool (*Test_Func)(void);

class TestManager
{
public:
	static TestManager& Get();
	void Add(class TestRoutine* test);
	void RunTests();

private:
	std::vector<class TestRoutine*> testList;
};

struct TestRoutine
{
	TestRoutine(const char* name, Test_Func func);
	std::string Name;
	Test_Func TestFuncPtr;
};
