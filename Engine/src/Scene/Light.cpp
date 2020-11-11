#include "Scene/Light.h"

Light::Light(const glm::vec3& pos, const float intensity, const glm::vec3& color, const float range)
	: _pos{ pos }, _intensity{ intensity}, _color{ color },
		_range{ range }, _ubo{ sizeof(float) * 8, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT }
{
	float data[]{ _pos.x, _pos.y, _pos.z, _intensity, 
		_color.x, _color.y, _color.z, _range,};
	_ubo.Map(&data, sizeof(float) * 8);
}