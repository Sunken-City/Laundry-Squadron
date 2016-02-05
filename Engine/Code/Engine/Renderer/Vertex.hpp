#pragma once

#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Renderer/RGBA.hpp"

struct Vertex_PCT
{
	Vertex_PCT() {};
	Vertex_PCT(const Vector3& position) : pos(position) {};
	Vertex_PCT(const Vector3& position, const RGBA& color) : pos(position), color(color) {};
	Vertex_PCT(const Vector3& position, const RGBA& color, const Vector2& texCoords) : pos(position), color(color), texCoords(texCoords) {};
	Vector3 pos;
	RGBA color;
	Vector2 texCoords;
};