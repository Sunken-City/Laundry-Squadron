#pragma once
#include "Engine/Math/Vector3.hpp"
#include <vector>
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Audio/Audio.hpp"


//-----------------------------------------------------------------------------
enum ParticleType { /*PARTICLE_AABB2,*/ PARTICLE_AABB3, PARTICLE_SPHERE }; //FUTURE IDEAS TODO: Add whatever TheRenderer supports as a Draw call!


//-----------------------------------------------------------------------------
class LinearDynamicsState;


//-----------------------------------------------------------------------------
struct Force
{
public:

	virtual Vector3 CalcForceForStateAndMass( const LinearDynamicsState* lds, float mass ) const = 0;
	virtual Force* GetCopy() const = 0;


protected:

	Force( float magnitude, Vector3 direction = Vector3::ZERO ) 
		: m_magnitude( magnitude )
		, m_direction( direction ) 
	{
	}

	float m_magnitude;
	Vector3 m_direction;

	virtual float CalcMagnitudeForState( const LinearDynamicsState* lds ) const { return m_magnitude; }
	virtual Vector3 CalcDirectionForState( const LinearDynamicsState* lds ) const { return m_direction; }
};


//-----------------------------------------------------------------------------
struct GravityForce : public Force // m*g
{
	GravityForce()
		: Force( 9.81f, -Vector3::UP )
	{
	}
	GravityForce( float magnitude, Vector3 direction = -Vector3::UP)
		: Force( magnitude, direction )
	{
	}

	Vector3 CalcForceForStateAndMass( const LinearDynamicsState* lds, float mass ) const override;
	Force* GetCopy() const { return new GravityForce( *this ); }
};


//-----------------------------------------------------------------------------
struct DebrisForce : public Force //The real problem: can't directly drag down velocity to zero, as lds is const.
{
	DebrisForce()
		: Force( 9.81f, -Vector3::UP), m_groundHeight( 0.f )
	{
	}
	DebrisForce( float magnitude, float groundHeight = 0.f, Vector3 direction = -Vector3::UP)
		: Force( magnitude, direction ), m_groundHeight( groundHeight )
	{
	}

	float m_groundHeight;

	Vector3 CalcForceForStateAndMass( const LinearDynamicsState* lds, float mass ) const override;
	float CalcMagnitudeForState( const LinearDynamicsState* lds ) const override; //Magnitude shrinks if you hit/sink below ground.
	virtual Vector3 CalcDirectionForState( const LinearDynamicsState* lds ) const override; //Direction inverts if you hit/sink below ground.
	Force* GetCopy() const { return new DebrisForce( *this ); }
};


//-----------------------------------------------------------------------------
struct ConstantWindForce : public Force // -c*(v - w)
{
	ConstantWindForce( float magnitude, Vector3 direction, float dampedness = 1.0f ) 
		: Force( magnitude, direction )
		, m_dampedness( dampedness ) 
	{ 
	}

	float m_dampedness; //"c".

	Vector3 CalcForceForStateAndMass( const LinearDynamicsState* lds, float mass ) const override;
	Force* GetCopy() const { return new ConstantWindForce( *this ); }
};


//-----------------------------------------------------------------------------
struct WormholeForce : public Force // -c*(v - w(pos))
{
	WormholeForce( float magnitude, Vector3 direction, float dampedness = 1.0f ) 
		: Force( magnitude, direction )
		, m_dampedness( dampedness ) 
	{
	}

	float m_dampedness; //"c".
	
	//Overriding to make wind = wind(pos).
	float CalcMagnitudeForState( const LinearDynamicsState* lds ) const override; //Further from origin you move == stronger the wind.
	virtual Vector3 CalcDirectionForState( const LinearDynamicsState* lds ) const override; //Direction sends you back toward origin.
	Vector3 CalcForceForStateAndMass( const LinearDynamicsState* lds, float mass ) const override;
	Force* GetCopy() const { return new WormholeForce( *this ); }
};


//-----------------------------------------------------------------------------
struct SpringForce : public Force // -cv + -kx
{
	SpringForce( float magnitude, Vector3 direction, float stiffness, float dampedness = 1.0f ) 
		: Force( magnitude, direction )
		, m_dampedness( dampedness )
		, m_stiffness( stiffness )
	{ 
	}

	float m_dampedness; //"c".
	float m_stiffness; //"k".

	Vector3 CalcForceForStateAndMass( const LinearDynamicsState* lds, float mass ) const override;
	Force* GetCopy() const { return new SpringForce( *this ); }
};


//-----------------------------------------------------------------------------
class LinearDynamicsState //Question: could these be attached to an EntityNode[3D]'s, and engine just make all entities + physics run in 3D?
{
public:

	LinearDynamicsState( Vector3 position = Vector3::ZERO, Vector3 velocity = Vector3::ZERO )
		: m_position( position )
		, m_velocity( velocity )
	{
	}
	~LinearDynamicsState();

	void StepWithForwardEuler( float mass, float deltaSeconds ); //Integrator invocation.
	Vector3 GetPosition() const { return m_position; }
	Vector3 GetVelocity() const { return m_velocity; }
	void SetPosition( const Vector3& newPos ) { m_position = newPos; }
	void SetVelocity( const Vector3& newVel ) { m_velocity = newVel; }
	void AddForce( Force* newForce ) { m_forces.push_back( newForce ); }
	void GetForces( std::vector< Force* >& out_forces ) { out_forces = m_forces; }


private:

	Vector3 m_position;
	Vector3 m_velocity;
	std::vector< Force* > m_forces; //i.e. All forces acting on whatever this LDS is attached to.

	LinearDynamicsState dStateForMass( float mass ) const; //Solves accel, for use in Step() integrators.
	Vector3 CalcNetForceForMass( float mass ) const; //Used by dState(), gets Sigma[F] by looping m_forces.
};


//-----------------------------------------------------------------------------
class Particle //NOTE: NOT an Entity3D because some entities won't need expiration logic.
{
public:

	Particle( ParticleType renderType, float mass, float secondsToLive, float renderRadius )
		: m_mass( mass )
		, m_secondsToLive( secondsToLive )
		, m_renderType( renderType )
		, m_renderRadius( renderRadius )
		, m_state( nullptr )
	{
	}
	~Particle();

	void GetParticleState( LinearDynamicsState& out_state ) const { out_state = *m_state; }
	void SetParticleState( LinearDynamicsState* newState ) { m_state = newState; }
	void Render();
	void StepAndAge( float deltaSeconds );
	bool IsExpired() const { return m_secondsToLive <= 0.f; }
	void GetForces( std::vector< Force* >& out_forces ) const;
	void AddForce( Force* newForce );
	void CloneForcesFromParticle( const Particle* sourceParticle );


private:

	float m_mass;
	float m_secondsToLive;

	ParticleType m_renderType;
	float m_renderRadius;

	LinearDynamicsState* m_state;
};


//-----------------------------------------------------------------------------
class ParticleSystem
{
public:

	ParticleSystem( Vector3 emitterPosition, ParticleType particleType, float particleRadius, float particleMass, 
					float muzzleSpeed, float maxDegreesDownFromWorldUp, float minDegreesDownFromWorldUp, float maxDegreesLeftFromWorldNorth, float minDegreesLeftFromWorldNorth,
					float secondsBetweenEmits, float secondsBeforeParticlesExpire, unsigned int maxParticlesEmitted, unsigned int particlesEmittedAtOnce )
		: m_emitterPosition( emitterPosition )
		, m_muzzleSpeed( muzzleSpeed )
		, m_maxDegreesDownFromWorldUp( maxDegreesDownFromWorldUp )
		, m_minDegreesDownFromWorldUp( minDegreesDownFromWorldUp )
		, m_maxDegreesLeftFromWorldNorth( maxDegreesLeftFromWorldNorth )
		, m_minDegreesLeftFromWorldNorth( minDegreesLeftFromWorldNorth )
		, m_particleToEmit( particleType, particleMass, secondsBeforeParticlesExpire, particleRadius )
		, m_secondsBetweenEmits( secondsBetweenEmits )
		, m_secondsBeforeParticlesExpire( secondsBeforeParticlesExpire )
		, m_maxParticlesEmitted( maxParticlesEmitted )
		, m_particlesEmittedAtOnce( particlesEmittedAtOnce )
		, m_secondsPassedSinceLastEmit( 0.f )
	{
		GUARANTEE_OR_DIE( m_particlesEmittedAtOnce <= m_maxParticlesEmitted, "Error in ParticleSystem ctor, amount to emit at once exceeds max amount to emit." ); //Else infinite loop in EmitParticles().
		m_particleToEmit.SetParticleState( new LinearDynamicsState( emitterPosition, Vector3::ZERO ) ); //So we can add forces to it prior to emission if requested.
		ParticleSystem::s_emitSoundID = AudioSystem::instance->CreateOrGetSound( "Data/Audio/Explo_EnergyFireball01.wav" );
	}
	~ParticleSystem();

	void RenderThenExpireParticles();
	void UpdateParticles( float deltaSeconds );
	void AddForce( Force* newForce ) { m_particleToEmit.AddForce( newForce ); }
	float GetSecondsUntilNextEmit() const { return m_secondsBetweenEmits - m_secondsPassedSinceLastEmit;  }

private:

	void StepAndAgeParticles( float deltaSeconds );
	void EmitParticles( float deltaSeconds ); //silently emits nothing if not yet time to emit.

	float m_maxDegreesDownFromWorldUp; //"theta" in most spherical-to-Cartesian conversions.
	float m_minDegreesDownFromWorldUp;
	float m_maxDegreesLeftFromWorldNorth; //"phi".
	float m_minDegreesLeftFromWorldNorth; 

	float m_muzzleSpeed; //How fast particles shoot out.
	float m_secondsPassedSinceLastEmit;
	float m_secondsBetweenEmits;
	float m_secondsBeforeParticlesExpire;
	unsigned int m_maxParticlesEmitted;
	unsigned int m_particlesEmittedAtOnce; //Destroys oldest one(s) on next emit until emitter can emit this amount. 
	//No angular velocity right now.
	//No ability to ignore parent velocity right now.

	Vector3 m_emitterPosition;

	Particle m_particleToEmit;
	std::vector< Particle* > m_unexpiredParticles;

	static const Vector3 MAX_PARTICLE_OFFSET_FROM_EMITTER;
	static SoundID s_emitSoundID;
};