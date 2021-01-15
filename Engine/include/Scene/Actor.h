#pragma once

#include "Transform.h"
#include "Mesh.h"
#include "VkRenderer/Material.h"

class Actor
{
public:
	Transform				_transform;

private:
	const Mesh*				_mesh		= nullptr;
	const MaterialInstance*	_material	= nullptr;

public:
	Actor(const Mesh& kMesh, const MaterialInstance& kMaterial);

public:
	void Draw(const CommandBuffer& commandBuffer) const;

};