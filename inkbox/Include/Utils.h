
#pragma once

#include <string>
#include <glm/vec3.hpp>

namespace utils
{
	std::string ReadFile(const char* path);

	bool LineIntersectsPlane(glm::vec3 lp, glm::vec3 ld, glm::vec3 pp, glm::vec3 pn, glm::vec3& intersection);
	bool LineIntersectsRect(glm::vec3 lp, glm::vec3 ld, glm::vec3 tl, glm::vec3 tr, glm::vec3 bl, glm::vec3 br, glm::vec3& intersection);
	bool LineIntersectsBox(glm::vec3 lp, glm::vec3 ld, glm::vec3 ttl, glm::vec3 ttr, glm::vec3 tbl, glm::vec3 tbr, glm::vec3 btl, glm::vec3 btr, glm::vec3 bbl, glm::vec3 bbr, glm::vec3& intersection);
	bool IsBetween(float x, float a, float b);
	bool ApproxEquals(float a, float b);
	bool ApproxEquals(float a, float b, float epsilon);

	bool StringEquals(const std::string& src, const char* other);
	bool StringStartsWith(const std::string& src, const char* start);
	bool StringEndsWith(const std::string& src, const char* end);
	int ParseNumericString(const std::string& src);
}