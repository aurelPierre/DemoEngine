#pragma once

#include <glm/vec3.hpp>

#include "VkRenderer/Buffer.h"

class Light
{
public:
	glm::vec3 _pos		= { 0.f, 0.f, 0.f };
	glm::vec3 _color	= { 1.f, 1.f, 1.f };
	glm::vec3 _range	= { 10.f, 10.f, 10.f };

	Buffer _ubo;

public:
	Light(const glm::vec3& pos, const glm::vec3& color, const glm::vec3& range);

public:

};