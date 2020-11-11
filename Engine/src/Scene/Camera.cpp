#include "Scene/Camera.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(const float fov, const float near, const float far)
	: _fov{ fov }, _near{ near }, _far{ far }, _ubo{ sizeof(glm::mat4) * 2, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT }
{
	Update();
}

void Camera::Update() const
{
	glm::vec3 cameraDir = _pos + glm::rotate(_rot, glm::vec3(0, 1, 0));
	glm::vec3 cameraUp = glm::rotate(_rot, glm::vec3(0, 0, 1));
	glm::mat4 view = glm::lookAt(_pos, cameraDir, cameraUp);

	glm::mat4 proj = glm::perspective(glm::radians(_fov), (16.f / 9.f), _near, _far);
	proj[1][1] *= -1;

	glm::mat4 data[]{ view, proj };

	_ubo.Map(&data, sizeof(glm::mat4) * 2);
}