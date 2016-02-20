#pragma once
#include "Engine/Audio/Audio.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Game/Projectile.hpp"
#include <vector>

class Texture;
class RGBA;
class Camera3D;
class Cloth;

class TheGame
{
public:
	//CONSTRUCTORS//////////////////////////////////////////////////////////////////////////
	TheGame();
	~TheGame();

	//FUNCTIONS//////////////////////////////////////////////////////////////////////////
	void Update(float deltaTime);
	void Render() const;
	void MoveCloth(float deltaTime);
	void UpdateCamera(float deltaTime);
	void SetUp3DPerspective() const;
	void RenderAxisLines() const;

	//STATIC VARIABLES//////////////////////////////////////////////////////////////////////////
	static TheGame* instance;
	static const Vector3 s_clothStartingPosition;

	//MEMBER VARIABLES////////////////////////////////////////////////////////////////////////////
	SoundID m_twahSFX;
	SoundID m_bgMusic;
	Cloth* m_cloth;
	Texture* m_marthTexture;
	std::vector<Projectile> m_projectiles;
private:
	RGBA* m_color;
	Camera3D* m_camera;
};
