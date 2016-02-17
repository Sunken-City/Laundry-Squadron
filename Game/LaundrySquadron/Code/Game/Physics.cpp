#include "Game/Physics.hpp"
#include "Engine/Renderer/TheRenderer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/AABB3.hpp"

#define STATIC 

//--------------------------------------------------------------------------------------------------------------
STATIC const Vector3 ParticleSystem::MAX_PARTICLE_OFFSET_FROM_EMITTER = Vector3::ZERO;
STATIC SoundID ParticleSystem::s_emitSoundID = 0;


//--------------------------------------------------------------------------------------------------------------
Particle::~Particle()
{
	if ( m_state != nullptr )
	{
		delete m_state;
		m_state = nullptr;
	}
}


//--------------------------------------------------------------------------------------------------------------
void Particle::Render()
{
	switch ( m_renderType )
	{
	case PARTICLE_SPHERE:
		TheRenderer::instance->DrawUVSphere(m_state->GetPosition(), m_renderRadius, 20.f);
		break;
	case PARTICLE_AABB3:
		Vector3 particlePos = m_state->GetPosition();
		Vector3 offsetToCorners = Vector3( m_renderRadius );
		Vector3 particleMins = particlePos - offsetToCorners;
		Vector3 particleMaxs = particlePos + offsetToCorners;
		//TheRenderer::instance->DrawShadedAABB( TheRenderer::VertexGroupingRule::AS_QUADS, AABB3( particleMins, particleMaxs ), Rgba::GREEN, Rgba::WHITE, Rgba::BLACK, Rgba::RED );
		TheRenderer::instance->DrawAABBBoundingBox(AABB3(particleMins, particleMaxs));
		break;
	//FUTURE IDEAS TODO: add more render types!
	}
}


//--------------------------------------------------------------------------------------------------------------
void Particle::StepAndAge( float deltaSeconds )
{
//	m_state->StepWithForwardEuler( m_mass, deltaSeconds );
	m_state->StepWithVerlet( m_mass, deltaSeconds );
	m_secondsToLive -= deltaSeconds;
}


//--------------------------------------------------------------------------------------------------------------
void Particle::GetForces( std::vector<Force*>& out_forces ) const
{
	if ( m_state != nullptr )
		m_state->GetForces( out_forces );
}


//--------------------------------------------------------------------------------------------------------------
void Particle::AddForce( Force* newForce )
{
	if ( m_state != nullptr ) 
		m_state->AddForce( newForce );
}


//--------------------------------------------------------------------------------------------------------------
void Particle::CloneForcesFromParticle( const Particle * sourceParticle )
{
	if ( m_state == nullptr ) return;

	std::vector< Force*> sourceParticleForces; 
	sourceParticle->GetForces( sourceParticleForces );

	for ( unsigned int forceIndex = 0; forceIndex < sourceParticleForces.size(); forceIndex++ )
		m_state->AddForce( sourceParticleForces[ forceIndex ]->GetCopy() );
}

//--------------------------------------------------------------------------------------------------------------
bool Particle::GetPosition( Vector3& out_position )
{
	if ( m_state == nullptr )
		return false;

	out_position = m_state->GetPosition();
	return true;
}


//--------------------------------------------------------------------------------------------------------------
bool Particle::SetPosition( const Vector3& newPosition )
{
	if ( m_state == nullptr )
		return false;

	m_state->SetPosition( newPosition );
	return true;
}


//--------------------------------------------------------------------------------------------------------------
bool Particle::Translate( const Vector3& translation )
{
	if ( m_state == nullptr )
		return false;

	m_state->SetPosition( m_state->GetPosition() + translation );
	return true;
}


//--------------------------------------------------------------------------------------------------------------
bool Particle::GetVelocity( Vector3& out_position )
{
	if ( m_state == nullptr )
		return false;

	out_position = m_state->GetVelocity();
	return true;
}


//--------------------------------------------------------------------------------------------------------------
bool Particle::SetVelocity( const Vector3& newVelocity )
{
	if ( m_state == nullptr )
		return false;

	m_state->SetVelocity( newVelocity );
	return true;
}


//--------------------------------------------------------------------------------------------------------------
LinearDynamicsState::~LinearDynamicsState()
{
	auto forceIterEnd = m_forces.cend();
	for ( auto forceIter = m_forces.cbegin(); forceIter != forceIterEnd; ++forceIter )
	{
		Force* currentForce = *forceIter;
		if ( currentForce != nullptr )
		{
			delete currentForce;
			currentForce = nullptr;
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
void LinearDynamicsState::StepWithForwardEuler( float mass, float deltaSeconds )
{
	//Forward Euler: state_next := state_prev + dt * d(state_prev)/dt.
	
	LinearDynamicsState dState = dStateForMass( mass );

	m_position += dState.m_position * deltaSeconds; //x := x + (veloc * dt)
	m_velocity += dState.m_velocity * deltaSeconds; //v := v + (accel * dt)
}

void LinearDynamicsState::StepWithVerlet( float mass, float deltaSeconds )
{
	//https://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet - to do away with the x_(t-1) at t=0 problem.
	static Vector3 prevAccel = 0.f;

	LinearDynamicsState dState = dStateForMass( mass );

	m_position += ( m_velocity*deltaSeconds ) + ( dState.m_velocity*.5f*deltaSeconds*deltaSeconds ); //x := x + v*dt + .5*a*dt*dt.
	m_velocity += ( prevAccel + dState.m_velocity )*.5f*deltaSeconds; //v := v + .5*(a + a_next)*dt.
	prevAccel = dState.m_velocity;
}


//--------------------------------------------------------------------------------------------------------------
LinearDynamicsState LinearDynamicsState::dStateForMass( float mass ) const
{
	Vector3 acceleration = CalcNetForceForMass( mass ) * ( 1.f / mass ); //Newton's 2nd law, rearranged.

	return LinearDynamicsState( m_velocity, acceleration );
}


//--------------------------------------------------------------------------------------------------------------
Vector3 LinearDynamicsState::CalcNetForceForMass( float mass ) const
{
	Vector3 netForce( 0.f );

	auto forceIterEnd = m_forces.cend();
	for ( auto forceIter = m_forces.cbegin(); forceIter != forceIterEnd; ++forceIter )
	{
		Force* currentForce = *forceIter;
		netForce += currentForce->CalcForceForStateAndMass( this, mass );
	}

	return netForce;
}


//--------------------------------------------------------------------------------------------------------------
Vector3 GravityForce::CalcForceForStateAndMass( const LinearDynamicsState * lds, float mass ) const
{
	return CalcDirectionForState( lds ) * CalcMagnitudeForState( lds ) * mass;
}


//--------------------------------------------------------------------------------------------------------------
Vector3 ConstantWindForce::CalcForceForStateAndMass( const LinearDynamicsState * lds, float /*mass*/ ) const
{
	Vector3 windVector = CalcDirectionForState( lds ) * CalcMagnitudeForState( lds );
	Vector3 undampedWindForce = lds->GetVelocity() - windVector;

	return undampedWindForce * -m_dampedness;
}


//--------------------------------------------------------------------------------------------------------------
float WormholeForce::CalcMagnitudeForState( const LinearDynamicsState * lds ) const
{
	return m_magnitude * lds->GetPosition().CalculateMagnitude(); //MAGIC: Further from origin you move == stronger wind.
}


//--------------------------------------------------------------------------------------------------------------
Vector3 WormholeForce::CalcDirectionForState( const LinearDynamicsState * lds ) const
{
	return Vector3::ZERO - lds->GetPosition(); //MAGIC: Direction sends you back toward origin.
}


//--------------------------------------------------------------------------------------------------------------
Vector3 WormholeForce::CalcForceForStateAndMass( const LinearDynamicsState* lds, float /*mass*/ ) const
{
	Vector3 windVector = CalcDirectionForState( lds ) * CalcMagnitudeForState( lds );
	Vector3 undampedWindForce = lds->GetVelocity() - windVector;

	return undampedWindForce * -m_dampedness;
}


//--------------------------------------------------------------------------------------------------------------
Vector3 SpringForce::CalcForceForStateAndMass( const LinearDynamicsState * lds, float /*mass*/ ) const
{
	Vector3 dampedVelocity = lds->GetVelocity() * -m_dampedness;
	Vector3 stiffenedPosition = lds->GetPosition() * -m_stiffness;

	return dampedVelocity + stiffenedPosition;
}


//--------------------------------------------------------------------------------------------------------------
ParticleSystem::~ParticleSystem()
{
	for ( unsigned int particleIndex = 0; particleIndex < m_unexpiredParticles.size(); particleIndex++ )
	{
		Particle* currentParticle = m_unexpiredParticles[ particleIndex ];
		if ( currentParticle != nullptr )
		{
			delete currentParticle;
			currentParticle = nullptr;
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
void ParticleSystem::RenderThenExpireParticles()
{
	for ( auto particleIter = m_unexpiredParticles.begin(); particleIter != m_unexpiredParticles.end(); )
	{
		Particle* currentParticle = *particleIter;
		currentParticle->Render();
		if ( currentParticle->IsExpired() )
		{
			delete currentParticle;
			currentParticle = nullptr;
			particleIter = m_unexpiredParticles.erase( particleIter );
		}
		else ++particleIter;
	}
}


//--------------------------------------------------------------------------------------------------------------
void ParticleSystem::UpdateParticles( float deltaSeconds )
{
	StepAndAgeParticles( deltaSeconds );
	EmitParticles( deltaSeconds );
}


//--------------------------------------------------------------------------------------------------------------
void ParticleSystem::StepAndAgeParticles( float deltaSeconds )
{
	for ( unsigned int particleIndex = 0; particleIndex < m_unexpiredParticles.size(); particleIndex++ )
	{
		m_unexpiredParticles[ particleIndex ]->StepAndAge( deltaSeconds );
	}
}


//--------------------------------------------------------------------------------------------------------------
void ParticleSystem::EmitParticles( float deltaSeconds )
{
	if ( m_secondsPassedSinceLastEmit >= m_secondsBetweenEmits )
	{
		m_secondsPassedSinceLastEmit = 0.f;

		//Prep for emit by erasing oldest particles to make enough room.
		unsigned int amountUnexpiredParticles = m_unexpiredParticles.size();

		while ( amountUnexpiredParticles + m_particlesEmittedAtOnce > m_maxParticlesEmitted )
		{
			//Remove earliest objects pushed--means removing from the front of a structure walked per-frame with items potentially being removed at its middle due their expiration. Any choice seems slow.
			m_unexpiredParticles.erase( m_unexpiredParticles.begin() );
			amountUnexpiredParticles = m_unexpiredParticles.size();
		}

		//Actual emit.
		for ( unsigned int iterationNum = 0; iterationNum < m_particlesEmittedAtOnce; iterationNum++ )
		{
			Particle* newParticle = new Particle( m_particleToEmit );
			Vector3 newParticlePosition = m_emitterPosition; //Below offset to position allows us not to just have particles emitting outward in "bands".
			newParticlePosition.x += MAX_PARTICLE_OFFSET_FROM_EMITTER.x * MathUtils::GetRandom(-1.f, 1.f);
			newParticlePosition.y += MAX_PARTICLE_OFFSET_FROM_EMITTER.y * MathUtils::GetRandom(-1.f, 1.f);
			newParticlePosition.z += MAX_PARTICLE_OFFSET_FROM_EMITTER.z * MathUtils::GetRandom(-1.f, 1.f);

			Vector3 muzzleVelocity; //Below follows spherical-to-Cartesian conversion formulas.
			float spanDegreesDownFromWorldUp = m_maxDegreesDownFromWorldUp - m_minDegreesDownFromWorldUp;
			float spanDegreesLeftFromWorldNorth = m_maxDegreesLeftFromWorldNorth - m_minDegreesLeftFromWorldNorth;
			muzzleVelocity.x = m_muzzleSpeed
				* MathUtils::SinDegrees( ( spanDegreesDownFromWorldUp		* MathUtils::GetRandomFromZeroTo(1.0f) ) + m_minDegreesDownFromWorldUp )
				* MathUtils::CosDegrees( ( spanDegreesLeftFromWorldNorth	* MathUtils::GetRandomFromZeroTo(1.0f)) + m_minDegreesLeftFromWorldNorth );
			muzzleVelocity.y = m_muzzleSpeed
				* MathUtils::SinDegrees( ( spanDegreesDownFromWorldUp		* MathUtils::GetRandomFromZeroTo(1.0f) ) + m_minDegreesDownFromWorldUp )
				* MathUtils::SinDegrees( ( spanDegreesLeftFromWorldNorth	* MathUtils::GetRandomFromZeroTo(1.0f) ) + m_minDegreesLeftFromWorldNorth );
			muzzleVelocity.z = m_muzzleSpeed
				* MathUtils::CosDegrees( ( spanDegreesDownFromWorldUp		* MathUtils::GetRandomFromZeroTo(1.0f)) + m_minDegreesDownFromWorldUp ); //Embeds assumption z is world-up? Would it work if using y-up, just rotated by 90deg?

			newParticle->SetParticleState( new LinearDynamicsState( newParticlePosition, muzzleVelocity ) );
			newParticle->CloneForcesFromParticle( &m_particleToEmit );
			m_unexpiredParticles.push_back( newParticle );
		}

		AudioSystem::instance->PlaySound( s_emitSoundID );
	}
	else m_secondsPassedSinceLastEmit += deltaSeconds;
}


//--------------------------------------------------------------------------------------------------------------
Vector3 DebrisForce::CalcForceForStateAndMass( const LinearDynamicsState * lds, float mass ) const
{
	return CalcDirectionForState( lds ) * CalcMagnitudeForState( lds ) * mass;
}


//--------------------------------------------------------------------------------------------------------------
float DebrisForce::CalcMagnitudeForState( const LinearDynamicsState * lds ) const
{
	float upComponentForPosition = MathUtils::Dot(lds->GetPosition(), Vector3::UP); //This way you don't have to embed z-up or y-up assumption.
	float upComponentForVelocity = MathUtils::Dot(lds->GetVelocity(), Vector3::UP);

	if ( upComponentForPosition < m_groundHeight )
		upComponentForPosition *= -10.f;
	else if ( upComponentForPosition > m_groundHeight && upComponentForVelocity < 0 )
		upComponentForPosition *= .65f;

	return upComponentForPosition;
}


//--------------------------------------------------------------------------------------------------------------
Vector3 DebrisForce::CalcDirectionForState( const LinearDynamicsState * lds ) const //The real problem: can't directly drag down velocity to zero.
{
	float upComponent = MathUtils::Dot(lds->GetPosition(), Vector3::UP); //This way you don't have to embed z-up or y-up assumption.

	//If upComponent is negative, we're below ground and need to invert direction with slightly less magnitude.
	if ( upComponent < m_groundHeight ) 
		return Vector3::UP;
	else 
		return -Vector3::UP;
}
