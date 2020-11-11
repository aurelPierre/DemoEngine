#pragma once

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>

#include "VkRenderer/Buffer.h"

#include "Editor.h"

class Camera
{
public:
	glm::vec3 _pos	= { 0.f, 0.f, 0.f };
	glm::quat _rot	= { 0.f, 0.f, 0.f, 1.f };

	float _fov	= 90.f;
	float _near = 0.1f;
	float _far	= 512.f;

	Buffer _ubo;

public:
	Camera(const float fov, const float near, const float far);

public:
	void Update() const;
};
