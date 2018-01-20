#include "ParticleEmitter.h"
#include "Particle.h"

#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <chrono>

#define PI2             6.28318530718f
#define THREADS         4

ofParameter< float >    ParticleEmitter::s_minSpeed{            "Min Part. Speed",     1.0f,   1.0f,  500.0f };
ofParameter< float >    ParticleEmitter::s_midSpeed{            "Max Part. Speed",    16.0f,   1.0f,  500.0f };
ofParameter< float >    ParticleEmitter::s_maxSpeed{            "Max Part. Speed",   100.0f,   1.0f,  500.0f };
ofParameter< float >    ParticleEmitter::s_functionStrength{    "Fn. Strentgth",       5.0f,   0.1f,   10.0f };
ofParameter< float >    ParticleEmitter::s_minParticleLife{     "Mix Part. Life",      1.0f,   0.5f,   60.0f };
ofParameter< float >    ParticleEmitter::s_maxParticleLife{     "Max Part. Life",     10.0f,   0.5f,   60.0f };
ofParameter< int   >    ParticleEmitter::s_particlesPerGroup{   "Particles/Group",     1000,    50,     5000 };
ofParameter< int   >    ParticleEmitter::s_particleGroups{      "Particle Groups",        1,     1,       20 };
ofParameter< bool  >    ParticleEmitter::s_debugDraw{           "Debug Draw",         false, false,     true };
ofParameterGroup        ParticleEmitter::s_emitterParams;

ofParameter< float >    ParticleEmitter::FuncCtl::s_minChangeTime{ "Min change time",  3.0f, 1.0f, 60.0f };
ofParameter< float >    ParticleEmitter::FuncCtl::s_maxChangeTime{ "Max change time", 20.0f, 1.0f, 60.0f };
ofParameterGroup        ParticleEmitter::FuncCtl::s_functionParams;

ofParameter< float >    ParticleEmitter::s_repelStrength{      "Repel Str.",    2.0f,    0.0f,     10.0f };
ofParameter< float >    ParticleEmitter::s_alignStrength{      "Align Str.",    4.0f,    0.0f,     10.0f };
ofParameter< float >    ParticleEmitter::s_attractStrength{    "Att. Str.",     8.0f,    0.0f,     10.0f };
ofParameter< float >    ParticleEmitter::s_zoneRadius{         "Area Size",   150.0f,   25.0f,    750.0f };
ofParameter< float >    ParticleEmitter::s_lowThresh{          "Repel Area",   0.45f,    0.0f,      1.0f };
ofParameter< float >    ParticleEmitter::s_highThresh{         "Align Area",   0.85f,    0.0f,      1.0f };
ofParameterGroup        ParticleEmitter::s_flockingParams;

void ParticleEmitter::init( void )
{
    Particle::init();
    
    if ( 0 == ParticleEmitter::FuncCtl::s_functionParams.size() )
    {
        ParticleEmitter::FuncCtl::s_functionParams.setName( "Func. Switch" );
        ParticleEmitter::FuncCtl::s_functionParams.add( ParticleEmitter::FuncCtl::s_minChangeTime, ParticleEmitter::FuncCtl::s_maxChangeTime );
    }
    
    if ( 0 == s_flockingParams.size() )
    {
        s_flockingParams.setName( "Flocking" );
        s_flockingParams.add( s_zoneRadius, s_repelStrength, s_alignStrength, s_attractStrength, s_lowThresh, s_highThresh );
    }
    
    if ( 0 == s_emitterParams.size() )
    {
        s_emitterParams.setName( "Emitter" );
        s_emitterParams.add( s_functionStrength, s_minParticleLife, s_maxParticleLife, s_particlesPerGroup, s_particleGroups, s_debugDraw );
        
    }
    
}

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
    m_referenceSurface( _surface ),
    m_stop( false ),
    m_pause( false ),
    m_processing( 0 ),
    m_currentTime( 0.0f ),
    m_delta( 0.0f ),
    m_currentDrawTime( 0.0f ),
    m_drawDelta( 0.0f ),
    m_updateFlockEvery( 0.1f ),
    m_updateFlockTimer( 0.0f ),
    m_lastFlockUpdateTime( 0.0f ),
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
    
    ofPoint displaySz   = ofPoint( 1280, 720 );
    
    // Flow setup
    m_flowWidth         = displaySz.x / 4;
    m_flowHeight        = displaySz.y / 4;
    
    // simulation setup
    m_opticalFlow.setup( m_flowWidth, m_flowHeight );
    m_opticalFlow.setStrength( 40.0f );
    m_opticalFlow.setOffset( 1.0f );
    m_opticalFlow.setLambda( 0.015f );
    m_opticalFlow.setThreshold( 0.01f );
    m_opticalFlow.setTimeBlurActive( true );
    m_opticalFlow.setTimeBlurRadius( 3.2f );
    m_opticalFlow.setTimeBlurDecay( 10.0f );
    
    // visualization setup
    m_scalarDisplay.setup( m_flowWidth, m_flowHeight );
    //m_scalarDisplay.allocate( m_flowWidth, m_flowHeight );
    m_velocityField.setup( m_flowWidth / 4, m_flowHeight / 4 );
    
    m_opticalFlowPixels.allocate( m_flowWidth, m_flowHeight, OF_IMAGE_COLOR_ALPHA );
    m_ftBo.allocate( 640, 480, GL_RGB32F );
    m_ftBo.black();
    
    m_scalarDisplay.setScale( 1.0f );
    
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
        m_particleMatrix.push_back( spatial_matrix< Particle >( 100, ofGetWidth(), ofGetHeight() ) );
    }
    
    auto& particleGroup = m_particles[ _group ];
    auto& particleMatrix = m_particleMatrix[ _group ];
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
        
        p->m_maxSpeedSquared = ofRandom( s_midSpeed, s_maxSpeed  );
        p->m_minSpeedSquared = ofRandom( s_minSpeed, s_midSpeed );
        
        p->m_acceleration         = p->m_direction;
        p->m_acceleration.normalize();
        p->m_acceleration        *= 2.5f;
        p->m_group                = _group;
        p->m_lifeTimeLeft         = ofRandom( ParticleEmitter::s_minParticleLife, ParticleEmitter::s_maxParticleLife );
        particleGroup.push_back( p );
        particleMatrix.insert( *p, p->m_position );
    }
}

void ParticleEmitter::draw( void )
{
    m_drawDelta = ofGetElapsedTimef() - m_currentDrawTime;
    m_currentDrawTime += m_drawDelta;
    
    for ( auto& particleGroup : m_particles )
    {
        for ( auto particle : particleGroup )
        {
            particle->draw();
        }
    }
}

void ParticleEmitter::drawOpticalFlow( void )
{
    ofPoint displaySz   = ofGetWindowSize();
    ofPushStyle();
    ofEnableBlendMode( OF_BLENDMODE_ALPHA );
    //ofFloatImage oi;
    //oi.setFromPixels( m_opticalFlowPixels );
    //oi.draw( m_position.x, m_position.y, m_referenceSurface->getWidth() * m_sizeFactor, m_referenceSurface->getHeight() * m_sizeFactor );
    m_scalarDisplay.setSource( m_opticalFlow.getOpticalFlowDecay() );
    m_scalarDisplay.draw( m_position.x, m_position.y, m_referenceSurface->getWidth() * m_sizeFactor, m_referenceSurface->getHeight() * m_sizeFactor );
    
    ofEnableBlendMode( OF_BLENDMODE_ADD );
    m_velocityField.setVelocity( m_opticalFlow.getOpticalFlowDecay() );
    m_velocityField.draw( m_position.x, m_position.y, m_referenceSurface->getWidth() * m_sizeFactor, m_referenceSurface->getHeight() * m_sizeFactor );
    ofPopStyle();
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
            m_particleMatrix.pop_back();
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
                particleGroup.back()->m_lifeTimeLeft = -1.0f;
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
    
    if ( m_updateFlockTimer >= m_updateFlockEvery )
    {
        m_updateFlockTimer    = 0.0;
        m_lastFlockUpdateTime = _currentTime;
        m_updateFlocking      = true;
    }
    
    // start threaded update
    startThreadedUpdate();
    // wait threaded update if it still pending
    waitThreadedUpdate();
}

void ParticleEmitter::updateVideo( bool _isNewFrame, ofVideoPlayer& _source, float _delta )
{
    if ( _isNewFrame && ( m_updateType & kOpticalFlow ) )
    {
        {
            m_ftBo.stretchIntoMe( _source.getTexture() );
            /*
            ofPushStyle();
            m_ftBo.begin();
            ofEnableBlendMode( OF_BLENDMODE_DISABLED );
            
            _source.draw( 0, 0, m_ftBo.getWidth(), m_ftBo.getHeight() );
            
            m_ftBo.end();
            ofPopStyle();
            */
        }
        
        //m_opticalFlow.setSource( _source.getTexture() );
        m_opticalFlow.setSource( m_ftBo.getTexture() );
        m_opticalFlow.update( _delta );
        updateOpticalFlow( _delta );
    }
}


void ParticleEmitter::updateOpticalFlow( float _delta )
{
    m_scalarDisplay.setSource( m_opticalFlow.getOpticalFlowDecay() );
    m_scalarDisplay.update();
    m_opticalFlow.getOpticalFlowDecay().readToPixels( m_opticalFlowPixels );
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

void ParticleEmitter::setInputArea( ofVec2f& _imageSize )
{
    //m_flowWidth         = _imageSize.x / 4;
    //m_flowHeight        = _imageSize.y / 4;
    //m_opticalFlow.setup( m_flowWidth, m_flowHeight );
    //m_scalarDisplay.setup( m_flowWidth, m_flowHeight );
    //m_velocityField.setup( m_flowWidth / 4, m_flowHeight / 4 );
    //m_opticalFlowPixels.allocate( m_flowWidth, m_flowHeight, OF_IMAGE_COLOR_ALPHA );
}

void ParticleEmitter::waitThreadedUpdate( void )
{
    std::unique_lock< std::mutex > ul( m_updateLock );
    
    // and wait until all threadsdo their job
    while ( !m_conditionVar.wait_for( ul, std::chrono::milliseconds( 10 ), [ this ](){ return m_processing == 0; } ) );
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
            auto& matrix    = m_particleMatrix[ groupIdx ];
            updateParticles( m_currentTime, m_delta, particles, matrix );
            
            groupIdx      += THREADS;
        }
        
        // Thread done - lock to update the processing count
        --m_processing;
        m_conditionVar.notify_one();
    }
}

void ParticleEmitter::updateParticles( float _currentTime, float _delta, std::vector< Particle* >& _particles, spatial_matrix< Particle >& _part_mtx )
{
    updateParticleTiming( _currentTime, _delta, _particles );
    
    if ( ( m_updateType & kFunction      ) != 0 ) updateParticlesFunctions(     _currentTime, _delta, _particles );
    if ( ( m_updateType & kFlocking      ) != 0 ) updateParticlesFlocking(      _currentTime, _delta, _particles, _part_mtx );
    if ( ( m_updateType & kFollowTheLead ) != 0 ) updateParticlesFollowTheLead( _currentTime, _delta, _particles, _part_mtx );
    if ( ( m_updateType & kOpticalFlow   ) != 0 ) updateParticlesOpticalFlow(   _currentTime, _delta, _particles );
    
    
    // TODO: to use this strategy a clear/insertion needs to be done on mode switch
    //if ( ( m_updateType & ( kFlocking | kFollowTheLead ) ) != 0 )
    {
        // if we're flocking we need to update the matrix
        _part_mtx.clear_apply( [&]( spatial_matrix< Particle >& mtx, Particle& p ){
            p.update( _currentTime, _delta, m_sizeFactor );
            if ( p.m_lifeTimeLeft > 0.0f ) {
                mtx.insert( p, p.m_position );
            }
        } );
    }
    /*
    else
    {
        // for non flocking, the matrix is not updated
        for ( auto p : _particles )
        {
            p->update( _currentTime, _delta, m_sizeFactor );
        }
    }
    */
    
    for ( int i = 0; i < _particles.size(); ++i )
    {
        Particle* p = _particles[ i ];
        if ( p->m_lifeTimeLeft < 0.0f )
        {
            delete p;
            _particles[ i ] = _particles[ _particles.size() - 1 ];
            _particles.pop_back();
            --i;
        }
    }
}

void ParticleEmitter::updateParticleTiming( float _currentTime, float _delta, std::vector< Particle* >& _particles )
{
    for ( int i = 0; i < _particles.size(); ++i )
    {
        Particle* p = _particles[ i ];
        p->updateTimer( _delta );
    }
}

void ParticleEmitter::updateParticlesFollowTheLead( float _currentTime, float _delta, std::vector< Particle* >& _particles, spatial_matrix< Particle >& _part_mtx )
{
    auto& p = _particles[ 0 ];
    ofVec2f& particleVelocity( p->m_velocity );
    ofVec2f& particlePosition( p->m_position );
    
    // update the position and velocity of the first particle
    ofVec2f force( m_xMathFunc( particlePosition.y / 25 )  - 0.5, m_yMathFunc( particlePosition.x / 25 )  - 0.5 );
    p->applyInstantForce( force * s_functionStrength );
    
    particleVelocity *= 1.0f + ( m_velocityAudioFunc( particlePosition.y ) * 5.0f );
    p->m_flockLeader = true;
    
    updateParticlesFlocking( _currentTime, _delta, _particles, _part_mtx );
    
    p->m_flockLeader = false;
}

void ParticleEmitter::updateParticlesFunctions( float _currentTime, float _delta, std::vector< Particle* >& _particles )
{
    // update the particles
    for ( auto& p : _particles )
    {
        ofVec2f& particleVelocity( p->m_velocity );
        ofVec2f& particlePosition( p->m_position );
        
        // update the position and velocity of each particle
        ofVec2f force(
            m_xMathFunc( particlePosition.y / 25 )  - 0.5,
            m_yMathFunc( particlePosition.x / 25 )  - 0.5
        );
        p->applyInstantForce( force  * s_functionStrength );
        
        particleVelocity *= 1.0f + ( m_velocityAudioFunc( particlePosition.y ) * 5.0f );
    }
}

void ParticleEmitter::updateParticlesFlocking( float _currentTime, float _delta, std::vector< Particle* >& _particles, spatial_matrix< Particle >& _part_mtx )
{
    // update the flocking routine
    if ( !m_updateFlocking )
    {
        return;
    }
    
    ofVec2f dir;
    float updateRatio    = static_cast< float >( ( _currentTime - m_lastFlockUpdateTime ) / m_updateFlockEvery );
    float zoneRadiusSqrd = s_zoneRadius * s_zoneRadius;
    
    for ( auto& p : _particles )
    {
        
        _part_mtx.apply_to_radius( [&]( spatial_matrix< Particle >& mtx, Particle& p1, Particle& p2 )
        {
            dir = p1.m_position - p2.m_position;
            float distSqrd = dir.lengthSquared();
            
            if ( distSqrd < zoneRadiusSqrd ) // Neighbor is in the zone
            {
                float percent = distSqrd / ( s_zoneRadius * s_zoneRadius );
                
                if( percent < s_lowThresh )            // Separation
                {
                    if ( s_repelStrength < 0.0001f )
                    {
                        return;
                    }
                    
                    float F = s_lowThresh * s_repelStrength * updateRatio;
                    dir.normalize();
                    dir *= F;
                    
                    if ( !p1.m_flockLeader ) p1.applyForce(  dir );
                    if ( !p2.m_flockLeader ) p2.applyForce( -dir );
                }
                else if( percent < s_highThresh ) // Alignment
                {
                    if ( s_alignStrength < 0.0001f )
                    {
                        return;
                    }
                    
                    float threshDelta     = s_highThresh - s_lowThresh;
                    float adjustedPercent = ( percent - s_lowThresh ) / threshDelta;
                    float F               = ( 1.0f - ( cos( adjustedPercent * PI2 ) * -0.5f + 0.5f ) ) * s_alignStrength * updateRatio;
                    
                    if ( !p1.m_flockLeader ) p1.applyForce( p2.m_direction * F );
                    if ( !p2.m_flockLeader ) p2.applyForce( p1.m_direction * F );
                    
                }
                else                                 // Cohesion
                {
                    if ( s_attractStrength < 0.0001f )
                    {
                        return;
                    }
                    
                    float threshDelta     = 1.0f - s_highThresh;
                    float adjustedPercent = ( percent - s_highThresh )/threshDelta;
                    float F               = ( 1.0f - ( cos( adjustedPercent * PI2 ) * -0.5f + 0.5f ) ) * s_attractStrength * updateRatio;
                    
                    dir.normalize();
                    dir *= F;
                    
                    if ( !p1.m_flockLeader ) p1.applyForce( -dir );
                    if ( !p2.m_flockLeader ) p2.applyForce(  dir );
                }
            }
            
        }, *p, p->m_position, s_zoneRadius );
    }
    
    
    /*while ( itr < itr_end )
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
                
                if ( distSqrd < s_zoneRadiusSqrd ) // Neighbor is in the zone
                {
                    float percent = distSqrd / s_zoneRadiusSqrd;
                    
                    if( percent < s_lowThresh )			// Separation
                    {
                        if ( s_repelStrength < 0.0001f )
                        {
                            continue;
                        }
                        
                        float F = s_lowThresh * s_repelStrength * updateRatio;
                        dir.normalize();
                        dir *= F;
                        
                        if ( !p1->m_flockLeader ) p1->applyForce( dir );
                        if ( !p2->m_flockLeader ) p2->applyForce( -dir );
                    }
                    else if( percent < s_highThresh ) // Alignment
                    {
                        if ( s_alignStrength < 0.0001f )
                        {
                            continue;
                        }
                        
                        float threshDelta     = s_highThresh - s_lowThresh;
                        float adjustedPercent	= ( percent - s_lowThresh ) / threshDelta;
                        float F               = ( 1.0f - ( cos( adjustedPercent * PI2 ) * -0.5f + 0.5f ) ) * s_alignStrength * updateRatio;
                        
                        if ( !p1->m_flockLeader ) p1->applyForce( p2->m_direction * F );
                        if ( !p2->m_flockLeader ) p2->applyForce( p1->m_direction * F );
                        
                    }
                    else 								// Cohesion
                    {
                        if ( s_attractStrength < 0.0001f )
                        {
                            continue;
                        }
                        
                        float threshDelta     = 1.0f - s_highThresh;
                        float adjustedPercent	= ( percent - s_highThresh )/threshDelta;
                        float F               = ( 1.0f - ( cos( adjustedPercent * PI2 ) * -0.5f + 0.5f ) ) * s_attractStrength * updateRatio;
                        
                        dir.normalize();
                        dir *= F;
                        
                        if ( !p1->m_flockLeader ) p1->applyForce( -dir );
                        if ( !p2->m_flockLeader ) p2->applyForce(  dir );
                    }
                }
            }
        }
        
        ++itr;
    }*/
}

void ParticleEmitter::updateParticlesOpticalFlow( float _currentTime, float _delta, std::vector< Particle* >& _particles )
{
    ofVec2f ratio( m_opticalFlowPixels.getWidth()  / ( m_referenceSurface->getWidth()  * m_sizeFactor ),
                   m_opticalFlowPixels.getHeight() / ( m_referenceSurface->getHeight() * m_sizeFactor ) );
    
    // update the particles
    for ( auto& p : _particles )
    {
        ofVec2f& particlePosition( p->m_position );
        ofFloatColor c = m_opticalFlowPixels.getColor( particlePosition.x * ratio.x, particlePosition.y * ratio.y );
        
        // update the position and velocity of each particle
        //ofVec2f force( ( static_cast< float >( c.r ) / 255.0f ) * ( 1.0f - static_cast< float >( c.a ) / 255.0f ),
        //               ( static_cast< float >( c.g ) / 255.0f ) * ( 1.0f - static_cast< float >( c.a ) / 255.0f ) );
        float multiplier = ( ( c.a ) * 10.0f );
        //multiplier *= multiplier;
        //multiplier /= 10.0f;
        ofVec2f force( ( c.r ) * multiplier,
                       ( c.g ) * multiplier );
        p->applyForce( force );
        //particleVelocity = force;
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
