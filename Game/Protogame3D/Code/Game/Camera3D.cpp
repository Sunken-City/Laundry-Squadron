#include "Game/Camera3D.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/TheRenderer.hpp"

Camera3D::Camera3D()
: m_orientation(0.f, 0.f, 0.f)
, m_position(8.f, 8.f, 100.f)
{
}

Vector3 Camera3D::GetForwardXYZ() const
{
	float cosYaw = MathUtils::CosDegrees(m_orientation.yawDegreesAboutZ);
	float sinYaw = MathUtils::SinDegrees(m_orientation.yawDegreesAboutZ);
	float cosPitch = MathUtils::CosDegrees(m_orientation.pitchDegreesAboutY);
	float sinPitch = MathUtils::SinDegrees(m_orientation.pitchDegreesAboutY);
	return Vector3(cosYaw * cosPitch, sinYaw * cosPitch, -sinPitch);
}

Vector3 Camera3D::GetForwardXY() const
{
	float cosYaw = MathUtils::CosDegrees(m_orientation.yawDegreesAboutZ);
	float sinYaw = MathUtils::SinDegrees(m_orientation.yawDegreesAboutZ);
	return Vector3(cosYaw, sinYaw, 0.f);
}

Vector3 Camera3D::GetLeftXY() const
{
	Vector3 forwardXY = GetForwardXY();
	return Vector3(-forwardXY.y, forwardXY.x, 0.f);
}

void Camera3D::UpdateViewFromCamera() const
{
	TheRenderer::instance->Rotate(-m_orientation.pitchDegreesAboutY, 0.f, 1.f, 0.f);
	TheRenderer::instance->Rotate(-m_orientation.yawDegreesAboutZ, 0.f, 0.f, 1.f);
	TheRenderer::instance->Rotate(-m_orientation.rollDegreesAboutX, 1.f, 0.f, 0.f);
	TheRenderer::instance->Translate(-m_position.x, -m_position.y, -m_position.z);
}
