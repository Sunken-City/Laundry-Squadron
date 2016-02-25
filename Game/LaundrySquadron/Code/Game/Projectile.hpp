#pragma once
#include "Game/Physics.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"


//-------------------------------------------------------------------------------------------------
class Projectile
{
public:
	//CONSTRUCTORS//////////////////////////////////////////////////////////////////////////
	Projectile(float mass, float radius, LinearDynamicsState const &state);
	~Projectile();

	//FUNCTIONS//////////////////////////////////////////////////////////////////////////
	void Update(float deltaSeconds);
	void Render() const;
	Vector3 GetPosition() const;
	void BackToPrevious();
	float IsColliding(Projectile const &ball1, Projectile const &ball2);
	void CollideAndBounce(Projectile *ball1, Projectile *ball2, float restitution, float deltaSeconds);

	//MEMBER VARIABLES//////////////////////////////////////////////////////////////////////////
	float m_mass;
	float m_radius;
	double m_birthday;
	LinearDynamicsState m_state;
	LinearDynamicsState m_prevState;
	bool m_collided;
};
