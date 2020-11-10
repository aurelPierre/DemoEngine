#pragma once

#include <vector>

#include "VkRenderer/Mesh.h"
#include "VkRenderer/Viewport.h"

struct Scene
{
	std::vector<Mesh*>		_mesh;
	std::vector<Viewport*>	_viewports;
};

void Draw(const LogicalDevice& kLogicalDevice, const Scene& scene);