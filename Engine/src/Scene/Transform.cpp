#include "Scene/Transform.h"

Transform::Transform()
	: _buffer{ sizeof(Mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT }
{
	Update();
}

void Transform::Update() const
{
	Mat4 transform = glm::translate(Mat4(1.f), _pos) * glm::toMat4(_rot) * glm::scale(Mat4(1.f), _scale);

	_buffer.Map(&transform, sizeof(Mat4));
}

void Transform::Translate(const Vec3& kPos, const Type kType)
{
	_pos += kType == Type::GLOBAL ? kPos : _rot * kPos;

	Update(); // TODO use dirty bool ?
}

void Transform::Rotate(const Vec3& kRot)
{
	_rot *= Quat(kRot);
}

void Transform::Rotate(const Quat& kRot)
{
	_rot *= kRot;

	Update(); // TODO use dirty bool ?
}

void Transform::Scale(const Vec3& kScale)
{
	_scale *= kScale;

	Update(); // TODO use dirty bool ?
}