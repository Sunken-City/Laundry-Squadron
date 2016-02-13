#pragma once
#include <vector>
#include "Engine/Renderer/RGBA.hpp"

class Vector3Int;
class Vector3;
class ShaderProgram;
class Material;
struct Vertex_PCUTB;

class Mesh
{
	typedef unsigned int GLuint;

public:
	Mesh();
	~Mesh();

	void RenderFromIBO(GLuint vaoID, const Material& program) const;
	static Mesh CreateCube(float sideLength, const RGBA& color = RGBA::WHITE);
	static Mesh CreateIcoSphere(float radius, const RGBA& color = RGBA::WHITE, int numPasses = 3);
	static Mesh CreateQuad(const Vector3& bottomLeft, const Vector3& topRight, const RGBA& color = RGBA::WHITE);

	GLuint m_vbo;
	GLuint m_ibo;

private:
	void Init();

	std::vector<Vertex_PCUTB> m_verts;
	std::vector<unsigned int> m_indices;
};