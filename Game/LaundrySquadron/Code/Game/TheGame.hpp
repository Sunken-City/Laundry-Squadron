#pragma once
#include "Engine/Audio/Audio.hpp"

class Texture;
class RGBA;
class Camera3D;

class TheGame
{
public:
	TheGame();
	~TheGame();
	void Update(float deltaTime);
	void UpdateCamera(float deltaTime);
	void Render() const;
	void SetUp3DPerspective() const;
	void RenderAxisLines() const;
	static TheGame* instance;

	SoundID m_twahSFX;
private:
	Texture* m_pauseTexture;
	RGBA* m_color;
	Camera3D* m_camera;
};