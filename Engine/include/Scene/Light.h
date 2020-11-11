#pragma once

#include <glm/vec3.hpp>

#include "VkRenderer/Buffer.h"

#include "Editor.h"

class Light
{
public:
	glm::vec3	_pos		= { 0.f, 0.f, 0.f };
	float		_intensity	= 1.f; 
	glm::vec3	_color		= { 1.f, 1.f, 1.f };
	float		_range		= 10.f;

	Buffer _ubo;

public:
	Light(const glm::vec3& pos, const float intensity, const glm::vec3& color, const float range);

public:

};