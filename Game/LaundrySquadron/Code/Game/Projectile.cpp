#include "Game/Projectile.hpp"
#include "Engine/Time/Time.hpp"
#include "Engine/Renderer/TheRenderer.hpp"


//-------------------------------------------------------------------------------------------------

Projectile::Projectile(float mass, float radius, LinearDynamicsState const &state)
	: m_mass(mass)
	, m_radius(radius)
	, m_state(state)
	, m_prevState(state)
	, m_collided(false)
	, m_birthday(GetCurrentTimeSeconds())
{

}


//-------------------------------------------------------------------------------------------------
Projectile::~Projectile()
{

}


//-------------------------------------------------------------------------------------------------
Vector3 Projectile::GetPosition() const
{
	return m_state.GetPosition();
}


//-------------------------------------------------------------------------------------------------
void Projectile::Update(float deltaSeconds)
{
	m_prevState = m_state;
	m_state.SetPosition(m_state.GetPosition() + m_prevState.GetVelocity() * deltaSeconds);
	m_state.SetVelocity(m_state.GetVelocity() + Vector3::ZERO);
}

void Projectile::Render() const
{
	TheRenderer::instance->DrawSexyOctohedron(m_state.GetPosition(), 1.0f, RGBA::VAPORWAVE);
}

//-------------------------------------------------------------------------------------------------
void Projectile::BackToPrevious()
{
	m_state = m_prevState;
}

//-------------------------------------------------------------------------------------------------
float Projectile::IsColliding(Projectile const &ball1, Projectile const &ball2)
{
	Vector3 x0 = ball2.m_prevState.GetPosition() - ball1.m_prevState.GetPosition();
	Vector3 e = ((ball2.m_state.GetPosition() - ball1.m_state.GetPosition()) - (x0));
	float R = ball1.m_radius + ball2.m_radius;
	float Dover4 = (MathUtils::Dot(x0, e) * MathUtils::Dot(x0, e)) - MathUtils::Dot(e, e) * (MathUtils::Dot(x0, x0) - R*R);
	if (Dover4 > 0)
	{
		float root1 = (-MathUtils::Dot(x0, e) - sqrt(Dover4)) / (MathUtils::Dot(e, e));
		float root2 = (-MathUtils::Dot(x0, e) + sqrt(Dover4)) / (MathUtils::Dot(e, e));
		if (root1 <= 1 && root2 >= 0)
		{
			root1 = root1 > 0.f ? root1 : 0.f;
			root2 = root2 < 1.f ? root2 : 1.f;

			return root1; //TRUE
		}
	}
	return -1.f; //FALSE
}

//-------------------------------------------------------------------------------------------------
void Projectile::CollideAndBounce(Projectile *ball1, Projectile *ball2, float restitution, float deltaSeconds)
{
	float enterTime = IsColliding(*ball1, *ball2);
	if (enterTime >= 0.00001f)
	{
		ball1->m_collided = true;
		ball2->m_collided = true;

		//Back up
		ball1->BackToPrevious();
		ball2->BackToPrevious();

		//Move to right before collide
		float timeUntilCollide = enterTime * deltaSeconds;
		ball1->Update(timeUntilCollide);
		ball2->Update(timeUntilCollide);

		//BOUNCE TIME
		Vector3 lineOfImpact = ball2->m_state.GetPosition() - ball1->m_state.GetPosition();
		lineOfImpact.Normalize();

		float Va = MathUtils::Dot(ball1->m_state.GetVelocity(), lineOfImpact);
		float Vb = MathUtils::Dot(ball2->m_state.GetVelocity(), lineOfImpact);

		float oneOverMasses = 1.f / (ball1->m_mass + ball2->m_mass);
		float massA = ball1->m_mass;
		float massB = ball2->m_mass;
		float VaPrime = oneOverMasses * (massA * Va + massB * Vb - massB * restitution * (Va - Vb));
		float VbPrime = oneOverMasses * (massA * Va + massB * Vb + massA * restitution * (Va - Vb));

		//State velocity reset	     NEW PARALLEL                 NEW PERPENDICULAR = (old perpindiclar)                       
		ball1->m_state.SetVelocity((lineOfImpact * VaPrime) + (ball1->m_state.GetVelocity() - lineOfImpact * Va));
		ball2->m_state.SetVelocity((lineOfImpact * VbPrime) + (ball2->m_state.GetVelocity() - lineOfImpact * Vb));

		float restOfTime = deltaSeconds - timeUntilCollide;
		ball1->Update(restOfTime);
		ball2->Update(restOfTime);
		//ASSERT_OR_DIE( IsColliding( *ball1, *ball2 ) < 0.f, "You are colliding, and you shouldn't be." )
	}
}