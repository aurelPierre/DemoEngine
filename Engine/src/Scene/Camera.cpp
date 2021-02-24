#include "Scene/Camera.h"

Camera::Camera(const float fov, const float near, const float far)
	: _fov{ fov }, _near{ near }, _far{ far }, _ubo{ sizeof(Mat4) * 2 + sizeof(Vec3), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT }
{
	Update();
}

void Camera::Update() const
{
	Vec3 cameraDir = _pos + glm::rotate(_rot, Vec3(0, 0, 1));
	Vec3 cameraUp = glm::rotate(_rot, Vec3(0, 1, 0));
	Mat4 view = glm::lookAt(_pos, cameraDir, cameraUp);

	Mat4 proj = glm::perspective(glm::radians(_fov), (16.f / 9.f), _near, _far);
	proj[1][1] *= -1;

	Mat4 data[]{ view, proj };

	_ubo.Map(&data, sizeof(Mat4) * 2);
	_ubo.Map((void*)&_pos, sizeof(Vec3), sizeof(Mat4) * 2);
}