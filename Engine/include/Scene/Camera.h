#pragma once

#include "VkRenderer/Buffer.h"

#include "Wrappers/glm.h"

#include "Editor.h"

class Camera
{
public:
	Vec3 _pos	= { 0.f, 0.f, 0.f };
	Quat _rot	= { 0.f, 0.f, 0.f, 1.f };

	float _fov	= 90.f;
	float _near = 0.1f;
	float _far	= 512.f;

	Buffer _ubo;

public:
	Camera(const float fov, const float near, const float far);

public:
	void Update() const;
};
