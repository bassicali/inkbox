
#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

class Camera
{
public:
	Camera();
	Camera(glm::vec3 pos, glm::vec3 target);
	void LookAt(glm::vec3 target);
	void Move(float offset);
	void OrbitY(float angle, glm::vec3 pivot);
	void OrbitX(float angle, glm::vec3 pivot);
	void Pan(glm::vec2 offset);

	glm::vec3 Position() const { return position; }
	glm::vec3 Direction() const { return direction; }
	glm::mat4 ViewMatrix() const { return view; }

private:
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 worldUp;
	glm::mat4 view;
};