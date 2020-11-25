#pragma once

#include "VkRenderer/Buffer.h"

#include "Editor.h"

#include "Wrappers/glm.h"

class Light
{
public:
	Vec3	_pos		= { 0.f, 0.f, 0.f };
	float	_intensity	= 1.f; 
	Vec3	_color		= { 1.f, 1.f, 1.f };
	float	_range		= 10.f;

	Buffer	_ubo;

public:
	Light(const Vec3& pos, const float intensity, const Vec3& color, const float range);

public:

};