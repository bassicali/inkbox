
#include "Tests.h"
#include "Utils.h"

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtx/rotate_vector.hpp>
#include <glm/geometric.hpp>

using namespace glm;
using namespace utils;

TestManager& TestManager::Get()
{
	static TestManager mgr;
	return mgr;
}

void TestManager::Add(class TestRoutine* test)
{
	testList.push_back(test);
}

void TestManager::RunTests()
{
	if (testList.size() == 0)
	{
		LOG_INFO("No tests to run");
		return;
	}

	LOG_INFO("Running %d tests", (int)testList.size());
	int passed = 0;
	for (TestRoutine* test : testList)
	{
		bool result = test->TestFuncPtr();
		LOG_INFO("%-50s %s", test->Name.c_str(), result ? "Pass" : "Fail");
		passed += result;
	}

	LOG_INFO("Results: %d/%d passed", passed, (int)testList.size());
}

TestRoutine::TestRoutine(const char* name, Test_Func func)
	: Name(name)
	, TestFuncPtr(func)
{
	TestManager::Get().Add(this);
}

DEFN_TEST(Line_Intersects_Rectangle)
{
	vec3 lo = vec3(0, 0, -2);
	vec3 ld = normalize(vec3(0, 0, 0) - lo);

	vec3 tl = vec3(-0.5, 0.5, 0);
	vec3 tr = vec3(0.5, 0.5, 0);
	vec3 bl = vec3(-0.5, -0.5, 0);
	vec3 br = vec3(0.5, -0.5, 0);

	vec3 x;
	bool intersects = LineIntersectsRect(lo, ld, tl, tr, bl, br, x);
	return intersects == true;
}

DEFN_TEST(Line_Intersects_Rectangle_Neg)
{
	vec3 lo = vec3(-1, 0, -2);
	vec3 ld = normalize(vec3(-1, 0, 0) - lo);

	vec3 tl = vec3(-0.5, 0.5, 0);
	vec3 tr = vec3(0.5, 0.5, 0);
	vec3 bl = vec3(-0.5, -0.5, 0);
	vec3 br = vec3(0.5, -0.5, 0);

	vec3 x;
	bool intersects = LineIntersectsRect(lo, ld, tl, tr, bl, br, x);
	return intersects == false;
}

DEFN_TEST(Line_Intersects_Box_Shoot_From_Front)
{
	vec3 lo = vec3(-0.25, 0, -2);
	vec3 ld = normalize(vec3(-0.25, 0, 0) - lo);

	vec3 ttl = vec3(-0.5, 0.5, 0.5);
	vec3 ttr = vec3(0.5, 0.5, 0.5);
	vec3 tbl = vec3(-0.5, 0.5, -0.5);
	vec3 tbr = vec3(0.5, 0.5, -0.5);

	vec3 btl = vec3(-0.5, -0.5, 0.5);
	vec3 btr = vec3(0.5, -0.5, 0.5);
	vec3 bbl = vec3(-0.5, -0.5, -0.5);
	vec3 bbr = vec3(0.5, -0.5, -0.5);

	vec3 i;
	bool intersects = LineIntersectsBox(lo, ld, ttl, ttr, tbl, tbr, btl, btr, bbl, bbr, i);

	return intersects == true && i.x == -0.25 && i.y == 0 && i.z == -0.5;
}

DEFN_TEST(Line_Intersects_Box_Shoot_From_Left)
{
	vec3 lo = vec3(-5, 0.25, 0);
	vec3 ld = normalize(vec3(-4, 0.25, 0) - lo);

	vec3 ttl = vec3(-0.5, 0.5, 0.5);
	vec3 ttr = vec3(0.5, 0.5, 0.5);
	vec3 tbl = vec3(-0.5, 0.5, -0.5);
	vec3 tbr = vec3(0.5, 0.5, -0.5);

	vec3 btl = vec3(-0.5, -0.5, 0.5);
	vec3 btr = vec3(0.5, -0.5, 0.5);
	vec3 bbl = vec3(-0.5, -0.5, -0.5);
	vec3 bbr = vec3(0.5, -0.5, -0.5);

	vec3 i;
	bool intersects = LineIntersectsBox(lo, ld, ttl, ttr, tbl, tbr, btl, btr, bbl, bbr, i);

	return intersects == true && i.x == -0.5 && i.y == 0.25 && i.z == 0;
}

