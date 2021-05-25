
#include "Camera.h"
#include "Common.h"
#include "Utils.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

using namespace glm;
using namespace utils;

Camera::Camera()
	: refUp(vec3(0.f, 1.f, 0.f))
	, position(vec3(0, 0, -2))
	//, position(vec3(-1.83, 1.43, 1.9))
	, direction(0)
	, up(0)
	, right(0)
	, view(0)
{
}

Camera::Camera(vec3 pos, vec3 target)
	: refUp(vec3(0.f, 1.f, 0.f))
	, position(pos)
	, direction(0)
	, up(0)
	, right(0)
	, view(0)
{
	direction = normalize(position - target);
	right = normalize(cross(refUp, direction));
	up = normalize(cross(direction, right));
}

void Camera::LookAt(glm::vec3 target) 
{
	direction = normalize(position - target);
	
	float x = dot(direction, refUp);
	if (ApproxEquals(x, 1.0f, 0.01f))
	{
		if (refUp.y == 1.0f)
		{
			refUp = vec3(0, 0, 1);
			//LOG_INFO("New refUp: <0, 0, 1>");
		}
		else if (refUp.z == 1)
		{
			refUp = vec3(0, -1, 0);
			//LOG_INFO("New refUp: <0, -1, 0>");
		}
		else if (refUp.y == -1)
		{
			refUp = vec3(0, 0, -1);
			//LOG_INFO("New refUp: <0, 0, -1>");
		}
		else if (refUp.z == -1)
		{
			refUp = vec3(0, 1, 0);
			//LOG_INFO("New refUp: <0, 1, 0>");
		}
	}

	right = normalize(cross(refUp, direction));
	up = normalize(cross(direction, right));
	view = lookAt(position, target, up);
}

void Camera::Move(float offset)
{
	if (direction == zero<vec3>())
		return;

	position -= direction * offset;
	view = lookAt(position, position - direction, refUp);
}

void Camera::OrbitX(float angle, glm::vec3 pivot)
{
	position = rotateX(position, angle);
	LookAt(pivot);
}

void Camera::OrbitY(float angle, glm::vec3 pivot)
{
	position = rotateY(position, angle);
	LookAt(pivot);
}

void Camera::Pan(glm::vec2 offset)
{
	if (offset == zero<vec2>())
		return;

	offset = 0.05f * normalize(offset);

	// LOG_INFO("PAN (%.3f, %.3f)", offset.x, offset.y);

	direction += right * offset.x;
	direction += up * offset.y;

	view = lookAt(position, position - direction, refUp);
}
