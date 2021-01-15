#pragma once

#include <vector>

#include "Scene/Actor.h"
#include "VkRenderer/Viewport.h"

struct Scene
{
	std::vector<Actor*>		_actors;
	std::vector<Viewport*>	_viewports;
};

void Draw(const Scene& scene);