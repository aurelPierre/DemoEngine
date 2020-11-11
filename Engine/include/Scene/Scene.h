#pragma once

#include <vector>

#include "Scene/Mesh.h"
#include "VkRenderer/Viewport.h"

struct Scene
{
	std::vector<Mesh*>		_mesh;
	std::vector<Viewport*>	_viewports;
};

void Draw(const Scene& scene);