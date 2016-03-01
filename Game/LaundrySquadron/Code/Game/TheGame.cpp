#include "Game/TheGame.hpp"
#include "Game/TheApp.hpp"
#include "Engine/Renderer/TheRenderer.hpp"
#include "Engine/Renderer/DebugRenderer.hpp"
#include "Engine/Renderer/AABB2.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/RGBA.hpp"
#include "Engine/Input/XInputController.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/Camera3D.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Input/Console.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Time/Time.hpp"
#include "Engine/Math/Noise.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Game/Physics.hpp"

#define WIN32_LEAN_AND_MEAN
#include<Windows.h>
#include <algorithm>

TheGame* TheGame::instance = nullptr;
const Vector3 TheGame::s_clothStartingPosition = Vector3(140.f, 20.f, 100.f);

//-----------------------------------------------------------------------------------
CONSOLE_COMMAND(twah)
{
	UNUSED( args );
	AudioSystem::instance->PlaySound(TheGame::instance->m_twahSFX);
}

//-----------------------------------------------------------------------------------
CONSOLE_COMMAND(resetCloth)
{
	UNUSED(args);
	delete TheGame::instance->m_cloth;
	TheGame::instance->m_cloth = new Cloth(TheGame::instance->s_clothStartingPosition, PARTICLE_AABB3, 1.f, .01f, 10, 10, 5, 1.f, sqrt(2.f), 2.f);
	AudioSystem::instance->PlaySound(TheGame::instance->m_startSFX);
	TheGame::instance->m_gameOver = false;
}

//-----------------------------------------------------------------------------------
TheGame::TheGame()
: m_marthTexture(Texture::CreateOrGetTexture("Data/Images/Test.png"))
, m_camera(new Camera3D())
, m_twahSFX(AudioSystem::instance->CreateOrGetSound("Data/SFX/twah.wav"))
, m_startSFX(AudioSystem::instance->CreateOrGetSound("Data/SFX/start.wav"))
, m_deathSFX(AudioSystem::instance->CreateOrGetSound("Data/SFX/death.wav"))
, m_bgMusic(AudioSystem::instance->CreateOrGetSound("Data/SFX/battleTheme.mp3"))
, m_cloth(new Cloth(s_clothStartingPosition, PARTICLE_AABB3, 1.f, .01f, 10, 10, 5, 1.f, sqrt(2.f), 2.f))
, m_timeSinceLastParticle(0.0f)
, m_gameOver(false)
{
	m_hurtSounds[0] = AudioSystem::instance->CreateOrGetSound("Data/SFX/hurt0.wav");
	m_hurtSounds[1] = AudioSystem::instance->CreateOrGetSound("Data/SFX/hurt1.wav");
	m_hurtSounds[2] = AudioSystem::instance->CreateOrGetSound("Data/SFX/hurt2.wav");
	m_hurtSounds[3] = AudioSystem::instance->CreateOrGetSound("Data/SFX/hurt3.wav");
	m_hurtSounds[4] = AudioSystem::instance->CreateOrGetSound("Data/SFX/hurt4.wav");
	Console::instance->RunCommand("motd");
	AudioSystem::instance->PlayLoopingSound(m_bgMusic); //There's no way to stop it c:
	AudioSystem::instance->PlaySound(m_startSFX);
}

//-----------------------------------------------------------------------------------
TheGame::~TheGame()
{
	delete m_cloth;
}

//-----------------------------------------------------------------------------------
void TheGame::Update(float deltaTime)
{
	if (InputSystem::instance->WasKeyJustPressed(InputSystem::ExtraKeys::TILDE))
	{
		Console::instance->ActivateConsole();
	}

	if (Console::instance->IsActive())
	{
		return; //Don't do anything involving input updates.
	}
	if (InputSystem::instance->WasKeyJustPressed('G'))
	{
		Console::instance->RunCommand("resetCloth");
	}
	m_timeSinceLastParticle += deltaTime;
	if (InputSystem::instance->IsKeyDown('Q'))
	{
		UpdateCamera(deltaTime);
	}
	else if (!m_gameOver)
	{
		MoveCloth(deltaTime);
	}
	if (m_numParticlesSpawned % 20 == 0 && InputSystem::instance->IsKeyDown('W'))
	{
		m_cloth->ResetForces(true);
		m_cloth->AddForce(new ConstantWindForce(GetPseudoRandomNoise1D(m_numParticlesSpawned) * 3000.0f, Vector3(MathUtils::GetRandom(-1.0f, 1.0f), MathUtils::GetRandom(-1.0f, 1.0f), MathUtils::GetRandom(-1.0f, 1.0f))));
	}
	m_cloth->Update(deltaTime);

	float timeForNextParticle = GetPseudoRandomNoise1D(m_numParticlesSpawned / 10);
	timeForNextParticle = MathUtils::RangeMap(timeForNextParticle, 0.0f, 1.0f, 0.0f, 0.5f);
	if (m_timeSinceLastParticle > timeForNextParticle)
	{
		const Vector3 BASE_VELOCITY = -Vector3::UNIT_Y * 10.0f;
		Vector3 velocity = BASE_VELOCITY;
		velocity += (Vector3::UNIT_X * MathUtils::GetRandom(-2.0f, 2.0f));
		velocity += (Vector3::UNIT_Z * MathUtils::GetRandom(-1.5f, 1.5f));
		m_projectiles.push_back(Projectile(1.0f, 0.5f, LinearDynamicsState(Vector3(144, 100, 96), velocity)));
		m_timeSinceLastParticle = 0.0f;
		m_numParticlesSpawned += 1;
	}

	bool gotHit = false;
	for (Projectile& bullet : m_projectiles)
	{
		bullet.Update(deltaTime);

		auto particleIterator = m_cloth->m_clothParticles.begin();
		while (particleIterator != m_cloth->m_clothParticles.end())
		{
			LinearDynamicsState* clothParticleState = (*particleIterator).m_state;
			Projectile clothParticle(1.0f, 0.1f, LinearDynamicsState(clothParticleState->GetPosition(), clothParticleState->GetVelocity()));
			float collisionFactor = bullet.IsColliding(bullet, clothParticle);
			if (collisionFactor != -1.0f)
			{
				particleIterator->SetIsExpired(true);
				gotHit = true;
			}
			++particleIterator;
		}

	}
	if (gotHit)
	{
		AudioSystem::instance->PlaySound(m_hurtSounds[MathUtils::GetRandom(0, 5)]);
	}
	m_projectiles.erase(std::remove_if(m_projectiles.begin(), m_projectiles.end(), [](Projectile& bullet) {return (bullet.m_collided || (GetCurrentTimeSeconds() - bullet.m_birthday) > 15.0f); }), m_projectiles.end());
	if (!m_gameOver && (m_cloth->IsDead() || m_cloth->GetPercentageConstraintsLeft() < 0.90f))
	{
		AudioSystem::instance->PlaySound(m_deathSFX);
		m_cloth->RemoveAllConstraints();
		m_cloth->AddForce(new GravityForce(100.0f));
		m_gameOver = true;
	}
}

//-----------------------------------------------------------------------------------
void TheGame::Render() const

{
	SetUp3DPerspective();
	m_camera->UpdateViewFromCamera();

	TheRenderer::instance->EnableDepthTest(true);
	TheRenderer::instance->DrawTexturedAABB(AABB2(Vector2(0.0f, 0.0f), Vector2(300.f, 300.f)), Vector2(1.0f, 1.0f), Vector2(0.0f, 0.0f), m_marthTexture, RGBA::WHITE);
	RenderAxisLines();

	m_cloth->Render(true, InputSystem::instance->IsKeyDown('C'), InputSystem::instance->IsKeyDown('C'));
	for (const Projectile& bullet : m_projectiles)
	{
		bullet.Render();
	}

	DebugRenderer::instance->Render();
	Console::instance->Render();
	TheRenderer::instance->SetOrtho(Vector2(0.0f, 0.0f), Vector2(1600.0f, 900.0f));
	//TheRenderer::instance->DrawTexturedAABB(AABB2(Vector2(0.0f, 0.0f), Vector2(1600.0f, 900.0f)), Vector2(1.0f, 1.0f), Vector2(0.0f, 0.0f), TheRenderer::instance->m_defaultTexture, RGBA(1.0f, 0.0f, 0.0f, 0.5f - ((m_cloth->GetPercentageConstraintsLeft() - 0.90f) / 0.20f)));
	if (m_gameOver)
	{
		TheRenderer::instance->DrawTexturedAABB(AABB2(Vector2(0.0f, 0.0f), Vector2(1600.0f, 900.0f)), Vector2(1.0f, 1.0f), Vector2(0.0f, 0.0f), TheRenderer::instance->m_defaultTexture, RGBA(1.0f, 0.0f, 0.0f, 0.5f));
		TheRenderer::instance->DrawText2D(Vector2(250.0f, 300.0f), "Great Job.", 10.0f, RGBA::WHITE, true, BitmapFont::CreateOrGetFontFromGlyphSheet("runescape"));
	}
	else
	{
		TheRenderer::instance->DrawTexturedAABB(AABB2(Vector2(0.0f, 0.0f), Vector2(1600.0f, 900.0f)), Vector2(1.0f, 1.0f), Vector2(0.0f, 0.0f), TheRenderer::instance->m_defaultTexture, RGBA(1.0f, 0.0f, 0.0f, 0.5f - ((m_cloth->GetPercentageConstraintsLeft() - 0.90f) / 0.20f)));
	}
	Vector3 position = m_camera->m_position;
	EulerAngles orientation = m_camera->m_orientation;
	//DebuggerPrintf("Camera Pos: (%f, %f, %f)   Camera Orientation: (%f, %f, %f)\n", position.x, position.y, position.z, orientation.rollDegreesAboutX, orientation.pitchDegreesAboutY, orientation.yawDegreesAboutZ);
}

//-----------------------------------------------------------------------------------
void TheGame::MoveCloth(float deltaTime)
{
	const float BASE_MOVE_SPEED = 4.5f;
	float moveSpeed;

	if (InputSystem::instance->IsKeyDown(VK_SHIFT))
	{
		moveSpeed = BASE_MOVE_SPEED * 8.0f;
	}
	else
	{
		moveSpeed = BASE_MOVE_SPEED;
	}
	if (InputSystem::instance->IsKeyDown('D') || InputSystem::instance->IsKeyDown(InputSystem::ExtraKeys::RIGHT))
	{
		Vector3 cameraLeftXY = m_camera->GetLeftXY();
		Vector3 maxOffset = (m_cloth->GetOriginalTopLeftPosition() + Vector3(16.0f, 0.0f, 0.0f));
		if (m_cloth->GetCurrentTopLeftPosition().x < maxOffset.x)
		{
			m_cloth->MoveClothByOffset(-cameraLeftXY * (moveSpeed * deltaTime));
		}
	}
	if (InputSystem::instance->IsKeyDown('A') || InputSystem::instance->IsKeyDown(InputSystem::ExtraKeys::LEFT))
	{
		Vector3 cameraLeftXY = m_camera->GetLeftXY();
		Vector3 maxOffset = (m_cloth->GetOriginalTopLeftPosition() - Vector3(16.0f, 0.0f, 0.0f));
		if (m_cloth->GetCurrentTopLeftPosition().x > maxOffset.x)
		{
			m_cloth->MoveClothByOffset(cameraLeftXY * (moveSpeed * deltaTime));
		}
	}
//	TheRenderer::instance->DrawText2D(Vector2(10, 10), Stringf("Current Top Left: %f %f %f", m_cloth->GetCurrentTopLeftPosition().x, m_cloth->GetCurrentTopLeftPosition().y, m_cloth->GetCurrentTopLeftPosition().z), 1.f);
	InputSystem::instance->HideMouseCursor();
}


//-----------------------------------------------------------------------------------
void TheGame::UpdateCamera(float deltaTime)
{
	const float BASE_MOVE_SPEED = 4.5f;
	float moveSpeed;

	if (InputSystem::instance->IsKeyDown(VK_SHIFT))
	{
		moveSpeed = BASE_MOVE_SPEED * 8.0f;
	}
	else
	{
		moveSpeed = BASE_MOVE_SPEED;
	}
	if (InputSystem::instance->IsKeyDown('W'))
	{
		Vector3 cameraForwardXY = m_camera->GetForwardXY();
		m_camera->m_position += cameraForwardXY * (moveSpeed * deltaTime);
	}
	if (InputSystem::instance->IsKeyDown('S'))
	{
		Vector3 cameraForwardXY = m_camera->GetForwardXY();
		m_camera->m_position -= cameraForwardXY * (moveSpeed * deltaTime);
	}
	if (InputSystem::instance->IsKeyDown('D'))
	{
		Vector3 cameraLeftXY = m_camera->GetLeftXY();
		m_camera->m_position -= cameraLeftXY * (moveSpeed * deltaTime);
	}
	if (InputSystem::instance->IsKeyDown('A'))
	{
		Vector3 camreaLeftXY = m_camera->GetLeftXY();
		m_camera->m_position += camreaLeftXY * (moveSpeed * deltaTime);
	}
	if (InputSystem::instance->IsKeyDown(' '))
	{
		m_camera->m_position += Vector3(0.f, 0.f, 1.f) * (moveSpeed * deltaTime);
	}
	if (InputSystem::instance->IsKeyDown('Z'))
	{
		m_camera->m_position += Vector3(0.f, 0.f, -1.f) * (moveSpeed * deltaTime);
	}

	InputSystem::instance->HideMouseCursor();
	Vector2Int cursorDelta = InputSystem::instance->GetDeltaMouse();

	m_camera->m_orientation.yawDegreesAboutZ -= ((float)cursorDelta.x * 0.022f);
	float proposedPitch = m_camera->m_orientation.pitchDegreesAboutY + ((float)cursorDelta.y * 0.022f);
	m_camera->m_orientation.pitchDegreesAboutY = MathUtils::Clamp(proposedPitch, -89.9f, 89.9f);
}

//-----------------------------------------------------------------------------------
void TheGame::SetUp3DPerspective() const
{
	const float aspect = 16.f / 9.f;
	const float nearDist = 0.1f;
	const float farDist = 1000.0f;
	const float fovY = 50.0f;
	TheRenderer::instance->SetPerspective(fovY, aspect, nearDist, farDist);

	//Put Z up.
	TheRenderer::instance->Rotate(-90.0f, 1.f, 0.f, 0.f);

	//Put X forward.
	TheRenderer::instance->Rotate(90.0f, 0.f, 0.f, 1.f);
}

//-----------------------------------------------------------------------------------
void TheGame::RenderAxisLines() const
{
	const float axisLineLength = 100.0f;
	TheRenderer::instance->EnableDepthTest(true);

	TheRenderer::instance->DrawLine(Vector3(0.0f, 0.0f, 0.0f), Vector3(axisLineLength, 0.0f, 0.0f), RGBA::RED, 3.0f);
	TheRenderer::instance->DrawLine(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, axisLineLength, 0.0f), RGBA::GREEN, 3.0f);
	TheRenderer::instance->DrawLine(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, axisLineLength), RGBA::BLUE, 3.0f);

	TheRenderer::instance->EnableDepthTest(false);

	TheRenderer::instance->DrawLine(Vector3(0.0f, 0.0f, 0.0f), Vector3(axisLineLength, 0.0f, 0.0f), RGBA::RED, 1.0f);
	TheRenderer::instance->DrawLine(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, axisLineLength, 0.0f), RGBA::GREEN, 1.0f);
	TheRenderer::instance->DrawLine(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, axisLineLength), RGBA::BLUE, 1.0f);

	TheRenderer::instance->EnableDepthTest(true);
}
