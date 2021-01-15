#include "Scene/Actor.h"

Actor::Actor(const Mesh& kMesh, const MaterialInstance& kMaterial)
	: _mesh{ &kMesh }, _material{ &kMaterial }
{
}

void Actor::Draw(const CommandBuffer& commandBuffer) const
{
	_material->Bind(commandBuffer);
	_mesh->Draw(commandBuffer);
}