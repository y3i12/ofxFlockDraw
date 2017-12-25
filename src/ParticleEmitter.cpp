#include "ParticleEmitter.h"
#include "Particle.h"

#include <cmath>
#include <algorithm>
#include <cstdlib>

#define PI2             6.28318530718f
#define THREADS         4

float ParticleEmitter::s_minParticleLife = 3.0f;
float ParticleEmitter::s_maxParticleLife = 10.0f;

int  ParticleEmitter::s_particlesPerGroup  = 0.0f;
int  ParticleEmitter::s_particleGroups     = 0.0f;
bool ParticleEmitter::s_debugDraw          = false;

float ParticleEmitter::FuncCtl::s_minChangeTime = 3.0f;
float ParticleEmitter::FuncCtl::s_maxChangeTime = 20.0f;

ParticleEmitter::FuncCtl::FuncCtl( std::vector< ParticleEmitter::PosFunc >& _fn ) :
    m_fnList( _fn ),
    m_funcTimer( 0.0f ),
    m_funcTimeout( 0.0f )
{
}

float ParticleEmitter::FuncCtl::operator()( float _v )
{
    float percent       = ofClamp( m_funcTimer / m_funcTimeout, 0.0f, 1.0f );
    float invPercent    = 1.0f - percent;
    float result = ( m_fnList[ m_fn[ 0 ] ]( _v ) * invPercent +
                     m_fnList[ m_fn[ 1 ] ]( _v ) * percent    );
    
    return std::isnan( result ) ? 0.0f : result;
    
}

void ParticleEmitter::FuncCtl::randomize( void )
{
    m_fn[ 0 ] = rand() % m_fnList.size();
    m_fn[ 1 ] = rand() % m_fnList.size();
}

void ParticleEmitter::FuncCtl::update( float _delta )
{
    // add the times to the timer
    m_funcTimer += _delta;
    
    // resets the timers if needed
    if ( m_funcTimer >= m_funcTimeout )
    {
        m_funcTimer     = 0.0;
        m_funcTimeout   = ofRandom( ParticleEmitter::FuncCtl::s_minChangeTime + _delta, ParticleEmitter::FuncCtl::s_maxChangeTime - _delta );
        m_fn[ 0 ]       = m_fn[ 1 ];
        m_fn[ 1 ]       = rand() % m_fnList.size();
    }
}

ParticleEmitter::ParticleEmitter( ofPixels*& _surface ) :
    m_position( 0.0f, 0.0f ),
    m_maxLifeTime( 0.0f ),
    m_minLifeTime( 0.0f ),
    m_soundLow( 0.0f ),
    m_soundMid( 0.0f ),
    m_soundHigh( 0.0f ),
    m_updateType( kFunctionAndFlocking ),
    m_zoneRadiusSqrd( 75.0f * 75.0f ),
    m_repelStrength( 0.04f ),
    m_alignStrength( 0.04f ),
    m_attractStrength( 0.02f ),
    m_lowThresh( 0.125f ),
    m_highThresh( 0.65f ),
    m_referenceSurface( _surface ),
    m_stop( false ),
    m_pause( false ),
    m_processing( 0 ),
    m_currentTime( 0.0 ),
    m_delta( 0.0 ),
    m_updateFlockEvery( 0.1 ),
    m_updateFlockTimer( 0.0 ),
    m_lastFlockUpdateTime( 0.0 ),
    m_particlesPerGroup( 0 ),
    m_particleGroups( 0 ),
    m_xMathFunc( m_mathFn ),
    m_velocityAudioFunc( m_audioFn ),
    m_yMathFunc( m_mathFn ),
    m_sizeFactor( 1.0f )
{
    for ( int i = 0; i < THREADS; ++i )
    {
        m_threads.push_back( std::thread( &ParticleEmitter::threadProcessParticles, this, i ) );
    }

    // Math related positioning functions
    /* 00 */ m_mathFn.push_back( &sinf );
    /* 01 */ m_mathFn.push_back( &cosf );
    /* 02 */ m_mathFn.push_back( &tanf );
    /* 03 */ m_mathFn.push_back( [ this ]( float x ){ return 1.0f; } );
    /* 04 */ m_mathFn.push_back( [ this ]( float x ){ return static_cast< float >( ( static_cast< int >( m_currentTime ) % 3 ) - 1 ); } );
    /* 05 */ m_mathFn.push_back( [ this ]( float x ){ return cosf( static_cast< float >( m_currentTime / 1e11 ) ); } );
    /* 06 */ m_mathFn.push_back( [ this ]( float x ){ return 5.0f / std::max< float >( fmod( x, 5.0f ), 0.1f ); } );
    /* 07 */ m_mathFn.push_back( [ this ]( float x ){ return fmod( x, 4.0f ) > 2.0f ? 1.0f : -1.0f; } );
    /* 08 */ m_mathFn.push_back( [ this ]( float x ){ return 1 / sinf( x ); } );
    /* 09 */ m_mathFn.push_back( [ this ]( float x ){ return sinf( x ) * tanf( std::min( x, 0.01f ) / 5 ); } );
    /* 10 */ m_mathFn.push_back( [ this ]( float x ){ return sinf( x ) * m_mathFn[ 9 ]( x ); } );
    /* 11 */ m_mathFn.push_back( [ this ]( float x ){ return static_cast< float >( ( static_cast< int >( m_currentTime + 7 * 13 ) % 3 ) - 1 ); } );

    //// Mirroring
#define GEN_NEGATIVE_FUNCTIONS( w, n ) w.push_back( [ this ]( float x ){ return w[ n ]( x ); } )
    GEN_NEGATIVE_FUNCTIONS( m_mathFn,  0 );
    GEN_NEGATIVE_FUNCTIONS( m_mathFn,  1 );
    GEN_NEGATIVE_FUNCTIONS( m_mathFn,  2 );
    GEN_NEGATIVE_FUNCTIONS( m_mathFn,  3 );
    GEN_NEGATIVE_FUNCTIONS( m_mathFn,  4 );
    GEN_NEGATIVE_FUNCTIONS( m_mathFn,  5 );
    GEN_NEGATIVE_FUNCTIONS( m_mathFn,  6 );
    GEN_NEGATIVE_FUNCTIONS( m_mathFn,  7 );
    GEN_NEGATIVE_FUNCTIONS( m_mathFn,  8 );
    GEN_NEGATIVE_FUNCTIONS( m_mathFn,  9 );
    GEN_NEGATIVE_FUNCTIONS( m_mathFn, 10 );
    GEN_NEGATIVE_FUNCTIONS( m_mathFn, 11 );
    
    
    m_xMathFunc.randomize();
    m_yMathFunc.randomize();
    
    // Audio Related Positioning Functions
    // m_audioFn
    /* 00 */ m_audioFn.push_back( [ this ]( float x ) { return  m_soundLow;   } );
    /* 02 */ m_audioFn.push_back( [ this ]( float x ) { return  m_soundMid;   } );
    /* 04 */ m_audioFn.push_back( [ this ]( float x ) { return  m_soundHigh;  } );
    
    m_velocityAudioFunc.randomize();
}

ParticleEmitter::~ParticleEmitter(void)
{
    killAll();
}

#define EMISSION_AREA_PERCENTAGE 1.0f
void ParticleEmitter::addParticles( int _group )
{
    ofVec2f      refSize( m_referenceSurface->getWidth() * m_sizeFactor, m_referenceSurface->getHeight() * m_sizeFactor );
    ofRectangle  emissionArea( m_position, m_position );
    
    // create groups as needed
    while ( m_particles.size() <= _group )
    {
        m_particles.push_back( std::vector< Particle* >() );
    }
    
    auto& particleGroup = m_particles[ _group ];
    int particlesToEmit = std::min< int >( 10, s_particlesPerGroup - particleGroup.size() );
    
    if ( m_referenceSurface )
    {
        emissionArea.x      = ofRandom( refSize.x - refSize.x * EMISSION_AREA_PERCENTAGE );
        emissionArea.y      = ofRandom( refSize.y - refSize.y * EMISSION_AREA_PERCENTAGE );
        emissionArea.width  = refSize.x * EMISSION_AREA_PERCENTAGE;
        emissionArea.height = refSize.y * EMISSION_AREA_PERCENTAGE;
    }
    
    float angle = ofRandom( 0.0f, 2 * PI );
    
    for ( int i = 0; i < particlesToEmit; ++i )
    {
        
        float   angleVar  = ofRandom( 0.0f, 0.8f * PI );
        ofVec2f angleVector( sin( angle + angleVar ), cos( angle + angleVar ) );
        
        Particle* p = 0;
        if ( m_referenceSurface )
        {
            ofVec2f pos;
            pos.x = ofRandom( emissionArea.x, emissionArea.x + emissionArea.width  );
            pos.y = ofRandom( emissionArea.y, emissionArea.y + emissionArea.height );
            
            p = new Particle( this, pos, angleVector );
            p->m_referenceSurface = m_referenceSurface;
        }
        else
        {
            p = new Particle( this, m_position, angleVector );
        }
        
        p->m_maxSpeedSquared = ofRandom( 10.0f, 50.0f );
        p->m_minSpeedSquared = ofRandom( 1.0f, 10.0f );
        
        p->m_acceleration         = p->m_direction;
        p->m_acceleration.normalize();
        p->m_acceleration        *= 2.5f;
        p->m_group                = _group;
        p->m_lifeTimeLeft         = ofRandom( ParticleEmitter::s_minParticleLife, ParticleEmitter::s_maxParticleLife );
        particleGroup.push_back( p );
    }
}

void ParticleEmitter::draw( void )
{
    for ( auto& particleGroup : m_particles )
    {
        for ( auto particle : particleGroup )
        {
            particle->draw();
        }
    }
}

void ParticleEmitter::debugDraw( void )
{
    if ( ParticleEmitter::s_debugDraw )
    {
        for ( auto& particleGroup : m_particles )
        {
            for ( auto particle : particleGroup )
            {
                particle->debugDraw();
            }
        }
    }
}

void ParticleEmitter::update( float _currentTime, float _delta )
{
    m_velocityAudioFunc.update( _delta );
    m_xMathFunc.update(  _delta );
    m_yMathFunc.update(  _delta );
    
    // add groups
    if ( m_particleGroups < s_particleGroups )
    {
        int groupsToCreate = s_particleGroups - m_particleGroups;
        for ( int i = 0; i < groupsToCreate; ++i ) {
            addParticles( m_particleGroups + i );
        }
        m_particleGroups += groupsToCreate;
    }
    // remove groups
    else if ( m_particleGroups > s_particleGroups )
    {
        int groupsToRemove = m_particleGroups - s_particleGroups;
        for ( int i = 0; i < groupsToRemove; ++i )
        {
            auto  particleGroupItr  = m_particles.rbegin();
            auto& particleGroup     = *particleGroupItr;
            
            for ( auto particle : particleGroup ) {
                delete particle;
            }
            
            m_particles.pop_back();
        }
        
        m_particleGroups = s_particleGroups;
    }
    
    // add particles
    if ( s_particlesPerGroup > m_particlesPerGroup )
    {
        size_t idx = 0;
        for ( auto& particleGroup : m_particles )
        {
            addParticles( idx++ );
        }
    }
    // remove particles
    else if ( m_particlesPerGroup > s_particlesPerGroup )
    {
        int particlesToRemove = m_particlesPerGroup - s_particlesPerGroup;
        
        for ( auto& particleGroup : m_particles )
        {
            for ( int i = 0; i < particlesToRemove; ++i )
            {
                delete particleGroup.back();
                particleGroup.pop_back();
            }
        }
        
        m_particlesPerGroup = s_particlesPerGroup;
    }
    
    // update timers
    m_updateFlockTimer += _delta;
    m_currentTime       = _currentTime;
    m_delta             = _delta;
 
    // update the flocking update timers
    if ( m_lastFlockUpdateTime == 0.0 )
    {
        m_lastFlockUpdateTime = m_currentTime - m_updateFlockEvery;
        m_updateFlockTimer    = m_updateFlockEvery;
    }

    // short circuit in case we don't have any particle group
    if ( m_particles.size() == 0 )
    {
        return;
    }
    
    
    // start threaded update
    startThreadedUpdate();
    // wait threaded update if it still pending
    waitThreadedUpdate();
}

void ParticleEmitter::startThreadedUpdate( void )
{
    // initialize the number of threads that are currently active
    size_t t_processing = std::min< size_t >( THREADS, m_particles.size() );
    m_processing = std::min< size_t >( THREADS, m_particles.size() );
    
    // notify all trheads to do their job
    for ( size_t i( 0 ); i < t_processing; ++i)
    {
        m_conditionVar.notify_one();
    }
}

void ParticleEmitter::waitThreadedUpdate( void )
{
    std::unique_lock< std::mutex > ul( m_updateLock );
    
    // and wait until all threadsdo their job
    m_conditionVar.wait( ul, [ this ](){ return m_processing == 0; } );
}

void ParticleEmitter::threadProcessParticles( size_t _threadNumber )
{
    while ( !m_stop )
    {
        std::unique_lock< std::mutex > cl( m_updateLock );
        // wait until we have something to process or we need to stop
        m_conditionVar.wait( cl, [ this ](){ return m_processing > 0 || m_stop; } );
        
        if ( m_pause || m_stop )
        {
            continue;
        }
        
        size_t groupIdx  = _threadNumber;
        size_t numGroups = m_particles.size();
        
        // process all particles that are pertinent to this tread
        while ( groupIdx < numGroups )
        {
            auto& particles = m_particles[ groupIdx ];
            updateParticles( m_currentTime, m_delta, particles );
            
            groupIdx      += THREADS;
        }
        
        // Thread done - lock to update the processing count
        --m_processing;
        m_conditionVar.notify_one();
    }
}

void ParticleEmitter::updateParticles( float _currentTime, float _delta, std::vector< Particle* >& _particles )
{
    updateParticleTiming( _currentTime, _delta, _particles );
    
    if ( ( m_updateType & kFunction      ) != 0 ) updateParticlesFunctions(     _currentTime, _delta, _particles );
    if ( ( m_updateType & kFlocking      ) != 0 ) updateParticlesFlocking(      _currentTime, _delta, _particles );
    if ( ( m_updateType & kFollowTheLead ) != 0 ) updateParticlesFollowTheLead( _currentTime, _delta, _particles );
    
    for ( auto p : _particles )
    {
        p->update( _currentTime, _delta, m_sizeFactor );
    }
}

void ParticleEmitter::updateParticleTiming( float _currentTime, float _delta, std::vector< Particle* >& _particles )
{
    for ( int i = 0; i < _particles.size(); ++i )
    {
        Particle* p = _particles[ i ];
        p->updateTimer( _delta );
        
        if ( p->m_lifeTimeLeft < 0.0f )
        {
            delete p;
            _particles[ i ] = _particles[ _particles.size() - 1 ];
            _particles.pop_back();
            --i;
        }
    }
}

void ParticleEmitter::updateParticlesFollowTheLead( float _currentTime, float _delta, std::vector< Particle* >& _particles )
{
    auto& p = _particles[ 0 ];
    ofVec2f& particleVelocity( p->m_velocity );
    ofVec2f& particlePosition( p->m_position );
    
    // update the position and velocity of the first particle
    ofVec2f force( m_xMathFunc( particlePosition.y / 25 )  - 0.5, m_yMathFunc( particlePosition.x / 25 )  - 0.5 );
    p->applyForce( force );
    
    particleVelocity *= 1.0f + ( m_velocityAudioFunc( particlePosition.y ) * 5.0f );
    p->m_flockLeader = true;
    
    updateParticlesFlocking( _currentTime, _delta, _particles );
    
    p->m_flockLeader = false;
}

void ParticleEmitter::updateParticlesFunctions( float _currentTime, float _delta, std::vector< Particle* >& _particles )
{
    // update the particles
    for ( auto& p : _particles )
    {
        ofVec2f& particleVelocity( p->m_velocity );
        ofVec2f& particlePosition( p->m_position );
        //ofVec2f& particleAcceleration( p->m_acceleration );
        
        // update the position and velocity of each particle
        ofVec2f force(
            m_xMathFunc( particlePosition.y / 25 )  - 0.5,
            m_yMathFunc( particlePosition.x / 25 )  - 0.5
        );
        p->applyForce( force );
        
        particleVelocity *= 1.0f + ( m_velocityAudioFunc( particlePosition.y ) * 5.0f );
    }
}

void ParticleEmitter::updateParticlesFlocking( float _currentTime, float _delta, std::vector< Particle* >& _particles )
{
    size_t  itr     = 0;
    size_t  itr_end = _particles.size();
    size_t  itr2;
    bool    updateFlock = false;
    float   updateRatio = 0.0f;
    ofVec2f dir;
    
    if ( m_updateFlockTimer >= m_updateFlockEvery )
    {
        m_updateFlockTimer    = 0.0;
        updateRatio           = static_cast< float >( ( _currentTime - m_lastFlockUpdateTime ) / m_updateFlockEvery );
        m_lastFlockUpdateTime = _currentTime;
        updateFlock           = true;
    }
    
    // update the flocking routine
    while ( itr < itr_end )
    {
        Particle* p1 = _particles[ itr ];
        
        if ( updateFlock )
        {
            itr2 = itr;
            
            for( ++itr2; itr2 != itr_end; ++itr2 )
            {
                Particle* p2 = _particles[ itr2 ];
                dir = p1->m_position - p2->m_position;
                float distSqrd = dir.lengthSquared();
                
                if ( distSqrd < m_zoneRadiusSqrd ) // Neighbor is in the zone
                {
                    float percent = distSqrd / m_zoneRadiusSqrd;
                    
                    if( percent < m_lowThresh )			// Separation
                    {
                        if ( m_repelStrength < 0.0001f )
                        {
                            continue;
                        }
                        
                        float F = m_lowThresh * m_repelStrength * updateRatio;
                        dir.normalize();
                        dir *= F;
                        
                        if ( !p1->m_flockLeader ) p1->m_acceleration += dir;
                        if ( !p2->m_flockLeader ) p2->m_acceleration -= dir;
                    }
                    else if( percent < m_highThresh ) // Alignment
                    {
                        if ( m_alignStrength < 0.0001f )
                        {
                            continue;
                        }
                        
                        float threshDelta     = m_highThresh - m_lowThresh;
                        float adjustedPercent	= ( percent - m_lowThresh ) / threshDelta;
                        float F               = ( 1.0f - ( cos( adjustedPercent * PI2 ) * -0.5f + 0.5f ) ) * m_alignStrength * updateRatio;
                        
                        if ( !p1->m_flockLeader ) p1->m_acceleration += p2->m_direction * F;
                        if ( !p2->m_flockLeader ) p2->m_acceleration += p1->m_direction * F;
                        
                    }
                    else 								// Cohesion
                    {
                        if ( m_attractStrength < 0.0001f )
                        {
                            continue;
                        }
                        
                        float threshDelta     = 1.0f - m_highThresh;
                        float adjustedPercent	= ( percent - m_highThresh )/threshDelta;
                        float F               = ( 1.0f - ( cos( adjustedPercent * PI2 ) * -0.5f + 0.5f ) ) * m_attractStrength * updateRatio;
                        
                        dir.normalize();
                        dir *= F;
                        
                        if ( !p1->m_flockLeader ) p1->m_acceleration -= dir;
                        if ( !p2->m_flockLeader ) p2->m_acceleration += dir;
                    }
                }
            }
        }
        
        ++itr;
    }
}

void ParticleEmitter::pauseThreads( void )
{
    m_pause = true;
}

void ParticleEmitter::continueThreads( void )
{
    m_pause = false;
}

void ParticleEmitter::killAll( void )
{
    std::unique_lock< std::mutex > cl( m_updateLock );
    m_pause = true;
    m_stop  = true;
    m_conditionVar.notify_all();
    cl.unlock();
    
    for ( auto& thread : m_threads )
    {
        thread.join();
    }
    m_threads.clear();
    
    m_stop  = false;
    m_pause = false;
    
    for ( auto& particleGroup : m_particles )
    {
        for ( auto particle : particleGroup ) {
            delete particle;
        }
    }
    m_particles.clear();
}
