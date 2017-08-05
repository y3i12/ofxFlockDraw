#include "ParticleEmitter.h"
#include "Particle.h"

#include <cmath>
#include <algorithm>

#define PI2             6.28318530718f
#define THREADS         4

bool ParticleEmitter::s_debugDraw          = false;
int  ParticleEmitter::s_particlesPerGroup  = 0.0f;
int  ParticleEmitter::s_particleGroups     = 0.0f;


ParticleEmitter::ParticleEmitter( ofPixels*& _surface ) :
    m_position( 0.0f, 0.0f ),
    m_maxLifeTime( 0.0f ),
    m_minLifeTime( 0.0f ),
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
    m_particleGroups( 0 )
{
    for ( int i = 0; i < THREADS; ++i )
    {
        m_threads.push_back( std::thread( &ParticleEmitter::threadProcessParticles, this, i ) );
    }
}

ParticleEmitter::~ParticleEmitter(void)
{
    killAll();
}

#define EMISSION_AREA_PERCENTAGE 0.3f
void ParticleEmitter::addParticles( int _aumont, int _group )
{
    ofVec2f      refSize( m_referenceSurface->getWidth(), m_referenceSurface->getHeight() );
    ofRectangle  emissionArea( m_position, m_position );
    
    // create groups as needed
    while ( m_particles.size() <= _group )
    {
        m_particles.push_back( std::vector< Particle* >() );
    }
    
    auto& particleGroup = m_particles[ _group ];
    
    if ( m_referenceSurface )
    {
        emissionArea.x      = ofRandom( refSize.x - refSize.x * EMISSION_AREA_PERCENTAGE );
        emissionArea.y      = ofRandom( refSize.y - refSize.y * EMISSION_AREA_PERCENTAGE );
        emissionArea.width  = refSize.x * EMISSION_AREA_PERCENTAGE;
        emissionArea.height = refSize.y * EMISSION_AREA_PERCENTAGE;
    }
    
    float angle = ofRandom( 0.0f, 2 * PI );
    
    for ( int i = 0; i < _aumont; ++i )
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
    // wait threaded update if it still pending
    waitThreadedUpdate();
    
    // add groups
    if ( m_particleGroups < s_particleGroups )
    {
        int groupsToCreate = s_particleGroups - m_particleGroups;
        for ( int i = 0; i < groupsToCreate; ++i ) {
            addParticles( m_particlesPerGroup, m_particleGroups + i );
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
        int particlesToEmit = std::min< int >( 2, s_particlesPerGroup - m_particlesPerGroup );
        
        size_t idx = 0;
        for ( auto& particleGroup : m_particles )
        {
            addParticles( particlesToEmit, idx++ );
        }
        
        m_particlesPerGroup += particlesToEmit;
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
}

void ParticleEmitter::startThreadedUpdate( void )
{
    // Create a lock to change our processing criteria (m_processing)
    std::unique_lock< std::mutex > ul( m_updateLock );
    
    // initialize the number of threads that are currently active
    m_processing = std::min< size_t >( THREADS, m_particles.size() );
    
    // notify all trheads to do their job
    for ( size_t i( 0 ), toNotify( m_processing ); i < m_processing; ++i)
    {
        m_startCondition.notify_one();
    }
}

void ParticleEmitter::waitThreadedUpdate( void )
{
    std::unique_lock< std::mutex > ul( m_updateLock );
    
    // and wait until all threadsdo their job
    m_doneCondition.wait( ul, [ this ](){ return m_processing == 0; } );
}

void ParticleEmitter::threadProcessParticles( size_t _threadNumber )
{
    while ( !m_stop )
    {
        std::unique_lock< std::mutex > cl( m_updateLock );
        // wait until we have something to process or we need to stop
        m_startCondition.wait( cl, [ this ](){ return m_processing > 0 || m_stop; } );
        
        if ( m_pause || m_stop )
        {
            continue;
        }
        
        size_t groupIdx  = _threadNumber;
        size_t numGroups = m_particles.size();
        
        // process all particles that are pertinent to this tread
        while ( groupIdx < numGroups )
        {
            auto particles = m_particles[ groupIdx ];
            updateParticles( m_currentTime, m_delta, particles );
            
            groupIdx      += THREADS;
        }
        
        // Thread done - lock to update the processing count
        --m_processing;
        m_doneCondition.notify_one();
    }
}

void ParticleEmitter::updateParticles( float _currentTime, float _delta, std::vector< Particle* >& _particles )
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
                        
                        p1->m_acceleration += dir;
                        p2->m_acceleration -= dir;
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
                        
                        p1->m_acceleration += p2->m_direction * F;
                        p2->m_acceleration += p1->m_direction * F;
                        
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
                        
                        p1->m_acceleration -= dir;
                        p2->m_acceleration += dir;
                    }
                }
            }
        }
        
        p1->update( _currentTime, _delta );
        
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
    m_startCondition.notify_all();
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
