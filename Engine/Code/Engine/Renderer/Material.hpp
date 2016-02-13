#pragma once

#include <string>

class ShaderProgram;
class Matrix4x4;

//STRUCTS//////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------------------------------------------------------------------
struct RenderState
{
	//ENUMS//////////////////////////////////////////////////////////////////////////
	enum class DepthTestingMode
	{
		ON,
		OFF,
		//XRAY,
		NUM_MODES
	};

	enum class FaceCullingMode
	{
		CULL_BACK_FACES,
		RENDER_BACK_FACES,
		NUM_MODES
	};

	enum class BlendMode
	{
		ADDITIVE_BLEND,
		ALPHA_BLEND,
		INVERTED_BLEND,
		NUM_MODES
	};
	
	//CONSTRUCTORS//////////////////////////////////////////////////////////////////////////
	RenderState(DepthTestingMode depthTesting = DepthTestingMode::ON, FaceCullingMode faceCulling = FaceCullingMode::CULL_BACK_FACES, BlendMode blendMode = BlendMode::ADDITIVE_BLEND);
	~RenderState();

	//FUNCTIONS//////////////////////////////////////////////////////////////////////////
	void SetState() const;
	void ClearState() const;

	//MEMBER VARIABLES//////////////////////////////////////////////////////////////////////////
	DepthTestingMode depthTestingMode;
	FaceCullingMode faceCullingMode;
	BlendMode blendMode;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------
class Material
{
public:
	//CONSTRUCTORS//////////////////////////////////////////////////////////////////////////
	Material() {};
	Material(ShaderProgram* program, const RenderState& renderState);
	Material(ShaderProgram* program, const std::string& diffusePath, const std::string& normalPath, const RenderState& renderState);
	~Material();

	//FUNCTIONS//////////////////////////////////////////////////////////////////////////
	void SetMatrices(const Matrix4x4& model, const Matrix4x4& view, const Matrix4x4& projection);
	void SetUpTextures() const;
	void SetUpRenderState() const;
	void CleanUpRenderState() const;
	//MEMBER VARIABLES//////////////////////////////////////////////////////////////////////////
	ShaderProgram* m_shaderProgram;
private:
	static unsigned int s_textureIDCounter;

	unsigned int m_samplerID;
	unsigned int m_diffuseID;
	unsigned int m_diffuseTexIndex;
	unsigned int m_normalID;
	unsigned int m_normalTexIndex;
	RenderState m_renderState;
};
