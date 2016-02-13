#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/ShaderProgram.hpp"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include "Engine/Renderer/OpenGLExtensions.hpp"

unsigned int Material::s_textureIDCounter = 0;

RenderState::RenderState(DepthTestingMode depthTesting, FaceCullingMode faceCulling, BlendMode blendMode)
	: depthTestingMode(depthTesting)
	, faceCullingMode(faceCulling)
	, blendMode(blendMode)
{

}

RenderState::~RenderState()
{
}

void RenderState::SetState() const
{
	switch (depthTestingMode)
	{
	case DepthTestingMode::OFF:
		Renderer::instance->EnableDepthTest(false);
		break;
	case DepthTestingMode::ON:
		Renderer::instance->EnableDepthTest(true);
		break;
	default:
		break;
	}
	switch (faceCullingMode)
	{
	case FaceCullingMode::CULL_BACK_FACES:
		Renderer::instance->EnableFaceCulling(true);
		break;
	case FaceCullingMode::RENDER_BACK_FACES:
		Renderer::instance->EnableFaceCulling(false);
		break;
	default:
		break;
	}
	switch (blendMode)
	{
	case BlendMode::ADDITIVE_BLEND:
		Renderer::instance->EnableAdditiveBlending();
		break;
	case BlendMode::ALPHA_BLEND:
		Renderer::instance->EnableAlphaBlending();
		break;
	case BlendMode::INVERTED_BLEND:
		Renderer::instance->EnableInvertedBlending();
		break;
	default:
		break;
	}
}

void RenderState::ClearState() const
{
	//Default state of the renderer.
	Renderer::instance->EnableDepthTest(true);
	Renderer::instance->EnableFaceCulling(true);
	Renderer::instance->EnableAlphaBlending();
}

Material::Material(ShaderProgram* program, const std::string& diffusePath, const std::string& normalPath, const RenderState& renderState)
	: m_shaderProgram(program)
	, m_diffuseID(Texture::CreateOrGetTexture(diffusePath)->m_openglTextureID)
	, m_normalID(Texture::CreateOrGetTexture(normalPath)->m_openglTextureID)
	, m_samplerID(Renderer::instance->CreateSampler(GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT))
	, m_renderState(renderState)
	, m_diffuseTexIndex(s_textureIDCounter++)
	, m_normalTexIndex(s_textureIDCounter++)
{
}

Material::Material(ShaderProgram* program, const RenderState& renderState) 
	: m_shaderProgram(program)
	, m_diffuseID(Renderer::instance->m_defaultTexture->m_openglTextureID)
	, m_normalID(Renderer::instance->m_defaultTexture->m_openglTextureID)
	, m_samplerID(Renderer::instance->CreateSampler(GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT))
	, m_renderState(renderState)
	, m_diffuseTexIndex(s_textureIDCounter++)
	, m_normalTexIndex(s_textureIDCounter++)
{

}

void Material::SetMatrices(const Matrix4x4& model, const Matrix4x4& view, const Matrix4x4& projection)
{
	m_shaderProgram->SetMatrix4x4Uniform("gModel", model);
	m_shaderProgram->SetMatrix4x4Uniform("gView", view);
	m_shaderProgram->SetMatrix4x4Uniform("gProj", projection);
}

void Material::SetUpTextures() const
{
	glActiveTexture(GL_TEXTURE0 + m_diffuseTexIndex);
	glBindTexture(GL_TEXTURE_2D, m_diffuseID);
	glBindSampler(0, m_samplerID);
	m_shaderProgram->SetIntUniform("gDiffuseTex", m_diffuseTexIndex);

	glActiveTexture(GL_TEXTURE0 + m_normalTexIndex);
	glBindTexture(GL_TEXTURE_2D, m_normalID);
	glBindSampler(0, m_samplerID);
	m_shaderProgram->SetIntUniform("gNormalTex", m_normalTexIndex);
}

void Material::SetUpRenderState() const
{
	m_renderState.SetState();
	glUseProgram(m_shaderProgram->m_shaderProgramID);
}

void Material::CleanUpRenderState() const
{
	m_renderState.ClearState();
}

