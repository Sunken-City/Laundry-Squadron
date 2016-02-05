#pragma once
#include <string>

class Vector3;
class Vector4;
class Matrix4x4;

class ShaderProgram
{
public:
	//Typedef'd so that we don't need to include Windows.h
	typedef unsigned int GLuint;
	typedef int GLint;
	typedef int GLsizei;
	typedef unsigned int GLenum;
	typedef bool GLboolean;

	//CONSTRUCTORS//////////////////////////////////////////////////////////////////////////
	ShaderProgram();
	ShaderProgram(const char* vertShaderPath, const char* fragShaderPath);
	~ShaderProgram();

	//FUNCTIONS//////////////////////////////////////////////////////////////////////////
	GLuint LoadShader(const char* filename, GLuint shader_type);
	void ShaderProgramBindProperty(const char *name, GLint count, GLenum type, GLboolean normalize, GLsizei stride, GLsizei offset);
	GLuint CreateAndLinkProgram(GLuint vs, GLuint fs);
	bool SetVec3Uniform(const char *name, const Vector3 &value);
	bool SetVec4Uniform(const char* name, const Vector4 &value);
	bool SetMatrix4x4Uniform(const char* name, const Matrix4x4 &value);
	bool SetIntUniform(const char* name, int value);
	bool SetFloatUniform(const char* name, float value);

	GLuint m_vertexShaderID;
	GLuint m_fragmentShaderID;
	GLuint m_shaderProgramID;
};