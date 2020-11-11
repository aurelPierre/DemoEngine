#include "Scene/Light.h"

Light::Light(const glm::vec3& pos, const glm::vec3& color, const glm::vec3& range)
	: _pos { pos }, _color{ color }, _range { range }, _ubo{ sizeof(glm::vec3) * 3, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT }
{
	glm::vec3 data[]{ pos, color, range };
	_ubo.Map(&data, sizeof(glm::vec3) * 3);
}