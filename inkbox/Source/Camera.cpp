
#include "Camera.h"
#include "Common.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

using namespace glm;

Camera::Camera()
	: worldUp(vec3(0.f, 1.f, 0.f))
	, position(vec3(0, 0, -2))
	//, position(vec3(-1.83, 1.43, 1.9))
	, direction(0)
	, up(0)
	, right(0)
	, view(0)
{
}

Camera::Camera(vec3 pos, vec3 target)
	: worldUp(vec3(0.f, 1.f, 0.f))
	, position(pos)
	, direction(0)
	, up(0)
	, right(0)
	, view(0)
{
	direction = normalize(position - target);
	right = normalize(cross(worldUp, direction));
	up = normalize(cross(direction, right));
}

void Camera::LookAt(glm::vec3 target) 
{
	direction = normalize(position - target);
	right = normalize(cross(worldUp, direction));
	up = normalize(cross(direction, right));
	view = lookAt(position, target, worldUp);
}

void Camera::Move(float offset)
{
	if (direction == zero<vec3>())
		return;

	position -= direction * offset;
	view = lookAt(position, position - direction, worldUp);
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

	view = lookAt(position, position - direction, worldUp);
}
