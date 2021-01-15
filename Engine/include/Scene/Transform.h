#pragma once

#include "Wrappers/glm.h"
#include "VkRenderer/Buffer.h"

class Transform
{
	enum class Type
	{
		LOCAL,
		GLOBAL
	};

	Vec3 _pos	= { 0.f, 0.f, 0.f };
	Quat _rot	= { 0.f, 0.f, 0.f, 1.f };
	Vec3 _scale = { 1.f, 1.f, 1.f };

public:
	Buffer _buffer;

public:
	Transform();

private:
	void Update() const;

public:
	void Translate(const Vec3& kPos, const Type kType = Type::LOCAL);
	void Rotate(const Vec3& kRot);
	void Rotate(const Quat& kRot);
	void Scale(const Vec3& kScale);
};