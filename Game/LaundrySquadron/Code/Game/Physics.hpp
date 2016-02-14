#pragma once
#include "Engine/Math/Vector3.hpp"
#include <vector>
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Audio/Audio.hpp"
#include "Engine/Renderer/TheRenderer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/AABB3.hpp"


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
	void DeleteState() { if ( m_state != nullptr ) delete m_state; }


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


//-----------------------------------------------------------------------------
enum ConstraintType {
	STRUCTURAL, SHEAR, BEND
};
struct ClothConstraint
{
	ConstraintType type;
	Particle* const p1;
	Particle* const p2;
	double restDistance; //How far apart p1, p2 are when cloth at rest.
	ClothConstraint( ConstraintType type, Particle* const p1, Particle* const p2, double restDistance )
		: type( type ), p1( p1 ), p2( p2 ), restDistance( restDistance ) {}
};


//-----------------------------------------------------------------------------
class Cloth
{

public:

	Cloth( const Vector3& originTopLeftPosition,
		   ParticleType particleRenderType, float particleMass, float particleRadius,
		   int numRows, int numCols,
		   unsigned int numConstraintSolverIterations,
		   double baseDistanceBetweenParticles,
		   double ratioDistanceStructuralToShear,
		   double ratioDistanceStructuralToBend,
		   const Vector3& initialGlobalVelocity = Vector3::ZERO )
		: m_originTopLeftPosition( originTopLeftPosition )
		, m_numRows( numRows )
		, m_numCols( numCols )
		, m_numConstraintSolverIterations( numConstraintSolverIterations )
		, m_baseDistanceBetweenParticles( baseDistanceBetweenParticles )
		, m_ratioDistanceStructuralToShear( ratioDistanceStructuralToShear )
		, m_ratioDistanceStructuralToBend( ratioDistanceStructuralToBend )
	{
		m_clothParticles.reserve( numRows * numCols );
		for ( int i = 0; i < numRows * numCols; i++ )
			m_clothParticles.push_back( Particle( particleRenderType, particleMass, -1.f, particleRadius ) ); //Doesn't assign a dynamics state.

		AssignParticleStates( baseDistanceBetweenParticles, originTopLeftPosition.z, initialGlobalVelocity );

		AddConstraints( baseDistanceBetweenParticles, ratioDistanceStructuralToShear, ratioDistanceStructuralToBend );
	}

	Particle* const GetParticle( int rowStartTop, int colStartLeft )
	{
		if ( rowStartTop > m_numRows )
			return nullptr;
		if ( colStartLeft > m_numCols )
			return nullptr;

		return &m_clothParticles[ ( rowStartTop * m_numRows ) + colStartLeft ]; //Row-major.
	}
	void Update( float deltaSeconds )
	{
		for ( int particleIndex = 0; particleIndex < m_numRows * m_numCols; particleIndex++ )
			m_clothParticles[ particleIndex ].StepAndAge( deltaSeconds );

		SatisfyConstraints();
	}
	void Render( bool showParticles = false )
	{
		//Render the cloth "fabric" by taking every 4 particle positions (r,c) to (r+1,c+1) in to make a quad.
		LinearDynamicsState particleStateTopLeft; //as 0,0 is top left. 
		LinearDynamicsState particleStateBottomRight;

		for ( int r = 0; ( r + 1 ) < m_numRows; r++ )
		{
			for ( int c = 0; ( c + 1 ) < m_numCols; c++ )
			{
				GetParticle( r, c )->GetParticleState( particleStateTopLeft );
				GetParticle( r + 1, c + 1 )->GetParticleState( particleStateBottomRight );
				AABB3 bounds( particleStateTopLeft.GetPosition(), particleStateBottomRight.GetPosition() );

				TheRenderer::instance->DrawTexturedAABB3( bounds, RGBA::WHITE, Vector2::ZERO, Vector2::ONE, Texture::CreateOrGetTexture("Data/Images/Test.png") );
			}
		}

		if ( !showParticles )
			return;

		for ( int particleIndex = 0; particleIndex < m_numRows * m_numCols; particleIndex++ )
			m_clothParticles[ particleIndex ].Render();
	}


private:

	void AssignParticleStates( float baseDistance, float nonPlanarDepth, const Vector3& velocity = Vector3::ZERO ) //Note: 0,0 == top-left, so +x is right, +y is down.
	{
		for ( int r = 0; r < m_numRows; r++ )
		{
			for ( int c = 0; c < m_numCols; c++ )
			{
				Vector3 startPosition( r * baseDistance, c * baseDistance, nonPlanarDepth );
				GetParticle( r, c )->SetParticleState( new LinearDynamicsState( startPosition, velocity ) );
			}
		}
	}

	void UpdateConstraints( ConstraintType affectedType, double newRestDistance )
	{
		for ( unsigned int constraintIndex = 0; constraintIndex < m_clothConstraints.size(); constraintIndex++ )
			if ( m_clothConstraints[ constraintIndex ].type == affectedType )
				m_clothConstraints[ constraintIndex ].restDistance = newRestDistance;
	}
	void AddConstraints( double baseDistance, double ratioStructuralToShear, double ratioStructuralToBend )
	{
		double shearDist = baseDistance * ratioStructuralToShear;
		double bendDist = baseDistance * ratioStructuralToBend;

		for ( int r = 0; r < m_numRows; r++ )
		{
			for ( int c = 0; c < m_numCols; c++ )
			{
				if ( r + 1 < m_numRows )
					m_clothConstraints.push_back( ClothConstraint( STRUCTURAL, GetParticle( r, c ), GetParticle( r + 1, c ), baseDistance ) );
				if ( ( r - 1 ) > 0 )
					m_clothConstraints.push_back( ClothConstraint( STRUCTURAL, GetParticle( r, c ), GetParticle( r - 1, c ), baseDistance ) );
				if ( ( c + 1 ) < m_numCols )
					m_clothConstraints.push_back( ClothConstraint( STRUCTURAL, GetParticle( r, c ), GetParticle( r, c + 1 ), baseDistance ) );
				if ( ( c - 1 ) > 0 )
					m_clothConstraints.push_back( ClothConstraint( STRUCTURAL, GetParticle( r, c ), GetParticle( r, c - 1 ), baseDistance ) );

				if ( ( r + 1 ) < m_numRows && ( c + 1 ) < m_numCols )
					m_clothConstraints.push_back( ClothConstraint( SHEAR, GetParticle( r, c ), GetParticle( r + 1, c + 1 ), shearDist ) );
				if ( ( r - 1 ) > 0 && ( c + 1 ) < m_numCols )
					m_clothConstraints.push_back( ClothConstraint( SHEAR, GetParticle( r, c ), GetParticle( r - 1, c + 1 ), shearDist ) );
				if ( ( r + 1 ) < m_numRows && ( c - 1 ) > 0 )
					m_clothConstraints.push_back( ClothConstraint( SHEAR, GetParticle( r, c ), GetParticle( r + 1, c - 1 ), shearDist ) );
				if ( ( r - 1 ) > 0 && ( c - 1 ) > 0 )
					m_clothConstraints.push_back( ClothConstraint( SHEAR, GetParticle( r, c ), GetParticle( r - 1, c - 1 ), shearDist ) );

				if ( ( r + 1 ) < m_numRows && ( c + 1 ) < m_numCols )
					m_clothConstraints.push_back( ClothConstraint( SHEAR, GetParticle( r, c ), GetParticle( r + 1, c + 1 ), bendDist ) );
				if ( ( r - 1 ) > 0 && ( c + 1 ) < m_numCols )
					m_clothConstraints.push_back( ClothConstraint( SHEAR, GetParticle( r, c ), GetParticle( r - 1, c + 1 ), bendDist ) );
				if ( ( r + 1 ) < m_numRows && ( c - 1 ) > 0 )
					m_clothConstraints.push_back( ClothConstraint( SHEAR, GetParticle( r, c ), GetParticle( r + 1, c - 1 ), bendDist ) );
				if ( ( r - 1 ) > 0 && ( c - 1 ) > 0 )
					m_clothConstraints.push_back( ClothConstraint( SHEAR, GetParticle( r, c ), GetParticle( r - 1, c - 1 ), bendDist ) );
			}
		}
	}
	void SatisfyConstraints()
	{
		for ( unsigned int numIteration = 0; numIteration < m_numConstraintSolverIterations; ++numIteration )
		{
			for ( unsigned int constraintIndex = 0; constraintIndex < m_clothConstraints.size(); constraintIndex++ )
			{
				ClothConstraint currentConstraint = m_clothConstraints[ constraintIndex ];

				LinearDynamicsState particleState2;
				LinearDynamicsState particleState1;

				currentConstraint.p1->GetParticleState( particleState1 );
				currentConstraint.p2->GetParticleState( particleState2 );

				Vector3 particlePosition1 = particleState1.GetPosition();
				Vector3 particlePosition2 = particleState2.GetPosition();
				Vector3 currentDisplacement = particlePosition2 - particlePosition1;

				if ( currentDisplacement == Vector3::ZERO )
					continue; //Skip solving for a step.
				double currentDistance = currentDisplacement.CalculateMagnitude();

				Vector3 halfCorrectionVector = currentDisplacement * 0.5 * ( 1.0 - ( currentConstraint.restDistance / currentDistance ) );
				// Note last term is ( currDist - currConstraint.restDist ) / currDist, just divided through.

				//Move p2 towards p1 (- along halfVec), p1 towards p2 (+ along halfVec).
				particleState1.SetPosition( particlePosition1 + halfCorrectionVector );
				particleState2.SetPosition( particlePosition2 - halfCorrectionVector );

				//Note: particles delete their own dynamics state's memory, but we have to clean up what we're replacing.
				currentConstraint.p1->DeleteState();
				currentConstraint.p2->DeleteState();
				currentConstraint.p1->SetParticleState( new LinearDynamicsState( particleState1 ) );
				currentConstraint.p2->SetParticleState( new LinearDynamicsState( particleState2 ) );
			}
		}
	}

	Vector3 m_originTopLeftPosition; //m_clothParticles[0,0].position.
	int m_numRows;
	int m_numCols;
	unsigned int m_numConstraintSolverIterations;

	//Ratios stored with class mostly for debugging. Or maybe use > these to tell when break a cloth constraint?
	double m_baseDistanceBetweenParticles;
	double m_ratioDistanceStructuralToShear;
	double m_ratioDistanceStructuralToBend;

	std::vector< Particle > m_clothParticles; //A 1D array, use GetParticle for 2D row-col interfacing accesses. Vector in case we want to push more at runtime.
	std::vector< ClothConstraint > m_clothConstraints; //TODO: make c-style after getting fixed-size formula given cloth dims?
};
