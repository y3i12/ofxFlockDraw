#include "Particle.h"
#include "ParticleEmitter.h"

#include <cmath>

//#define DEG2RAD( x ) ( ( x ) * 0.017453292519943295769236907684886f )
//#define RAD2DEG( x ) ( ( x ) * 57.2958f )
#define HPI ( PI / 2 )
#define LUMINANCE( r, g, b ) ( 0.299f * ( r ) + 0.587f * ( g ) + 0.114f * ( b ) )

#define WRAP( p, w )                                \
if ( p.x < 0.0f )       p.x += w.x;                 \
else if ( p.x >= w.x )  p.x  = fmod( p.x, w.x );    \
if ( p.y < 0.0f )       p.y += w.y;                 \
else if ( p.y >= w.y )  p.y  = fmod( p.y, w.y );

float  Particle::s_maxRadius          = 0.015f;
float  Particle::s_particleSizeRatio  = 1.0f;
float  Particle::s_particleSpeedRatio = 1.0f;
float  Particle::s_dampness           = 0.9f;
float  Particle::s_colorRedirection   = 1.0f;
size_t Particle::s_idGenerator        = 0;

Particle::Particle( ParticleEmitter* _owner, ofVec2f& _position, ofVec2f& _direction ) :
    m_position( _position ),
    m_stablePosition( _position ),
    m_direction( _direction ),
    m_color( 255, 255, 255 ),
    m_alpha( 0 ),
    m_velocity( 0.0f, 0.0f ),
    m_maxSpeedSquared( 0.0f ),
    m_minSpeedSquared( 0.0f ),
    m_lifeTime( 0.0f ),
    m_lifeTimeLeft( 0.0f ),
    m_flockLeader( false ),
    m_owner( _owner ),
    m_group( -1 ),
    m_id( s_idGenerator++ ),
    m_referenceSurface( _owner->m_referenceSurface )
{
}

Particle::~Particle( void )
{
}

void Particle::applyForce( ofVec2f _force, bool _limit )
{
    m_velocity += _force;
    m_direction = m_velocity.getNormalized();
    if ( _limit ) limitSpeed();
}

void Particle::update( float _currentTime, float _delta )
{

    // capping to avoid errors
    if ( std::isnan( m_velocity.x ) || fabs( m_velocity.x ) > 100.0f ||
         std::isnan( m_velocity.y ) || fabs( m_velocity.y ) > 100.0f )
    {
        m_velocity.normalize();
    }
    
    // update the speed
    m_velocity += m_acceleration;
    m_acceleration.set( 0.0f, 0.0f );
    m_direction = m_velocity.getNormalized();
    //limitSpeed();
    
    // update the position
    m_position += m_velocity * _delta * Particle::s_particleSpeedRatio;
    m_velocity *= Particle::s_dampness;
    
    
    // capping to avoid errors
    if ( std::isnan( m_velocity.x ) || m_velocity.x > 500.0f ) m_velocity.x = 0.0f;
    if ( std::isnan( m_velocity.y ) || m_velocity.y > 500.0f ) m_velocity.y = 0.0f;
    if ( std::isnan( m_position.x ) || m_position.x <   0.0f ) m_position.x = 0.0f;
    if ( std::isnan( m_position.y ) || m_position.y <   0.0f ) m_position.y = 0.0f;
    
    if ( m_referenceSurface )
    {
        // wrap the particle
        ofVec2f wrapSize( m_referenceSurface->getWidth(), m_referenceSurface->getHeight() );
        
        WRAP( m_position, wrapSize );
        
        t_tempDir = m_direction * 2.0f;
        t_angle   = 45;
        
        t_currentColor = m_referenceSurface->getColor( static_cast< int >( m_position.x ), static_cast< int >( m_position.y ) );
        
        t_nextPos[ 0 ] = m_position + t_tempDir;
        t_tempDir.rotate( t_angle );
        t_nextPos[ 1 ] = m_position + t_tempDir;
        t_tempDir.rotate( t_angle * -2.0f );
        t_nextPos[ 2 ] = m_position + t_tempDir;
        
        // image guidance
        for ( int i = 0; i < 3; ++i )
        {
            ofVec2f& pointRef = t_nextPos[ i ];
            
            WRAP( pointRef, wrapSize );
            
            // to guide thru color
            t_c      = t_currentColor - m_referenceSurface->getColor(
                std::max< int >( static_cast< int >( pointRef.x ), 0 ),
                std::max< int >( static_cast< int >( pointRef.y ), 0 )
            );
            t_l[ i ] = t_c.r * 2.0f + t_c.g * 2.0f + t_c.b * 2.0f;
            
            // to guide thru luminance
            // ci::ColorA c  = m_referenceSurface->getPixel( nextPos[ i ] );
            // l[ i ] = LUMINANCE( c.r, c.g, c.b );
        }
        
        t_angle = Particle::s_colorRedirection;

        if ( t_l[ 1 ] < t_l[ 0 ] )
        {
            m_velocity.rotate( t_angle * _delta );
        }
        else if ( t_l[ 2 ] < t_l[ 0 ] )
        {
            m_velocity.rotate( t_angle * -2.0f * _delta );
        }
    }
}

void Particle::updateTimer( float _delta )
{
    m_lifeTime     += _delta;
    m_lifeTimeLeft -= _delta;
    
    // fade in and out
    if ( m_lifeTime < 1.0f )
    {
        m_alpha = static_cast< unsigned char >( 255 * m_lifeTime );
    }
    else if ( m_lifeTimeLeft < 1.0f )
    {
        m_alpha = static_cast< unsigned char >( 255 * m_lifeTimeLeft );
    }
}

void Particle::draw( void )
{
    //t_sourceArea.x1 = static_cast< int >( m_position.x - Particle::s_maxRadius );
    //t_sourceArea.y1 = static_cast< int >( m_position.y - Particle::s_maxRadius );
    //t_sourceArea.x2 = static_cast< int >( m_position.x + Particle::s_maxRadius );
    //t_sourceArea.y2 = static_cast< int >( m_position.y + Particle::s_maxRadius );
    // TODO: get particle color at the area
    
    t_color      = m_referenceSurface->getColor( static_cast< int >( m_position.x ), static_cast< int >( m_position.y ) );
    m_color = m_color / 2 + t_color / 2;
    float radius = ( 1.0f + Particle::s_maxRadius * LUMINANCE( t_color.r, t_color.g, t_color.b ) ) * Particle::s_particleSizeRatio;
    
    ofSetColor( m_color.r, m_color.g, m_color.b, m_alpha );
    ofFill();
    ofDrawCircle( m_position + m_owner->m_position, radius );
}

void Particle::debugDraw( void )
{
    ofNoFill();
    float zoneRadius = sqrt( m_owner->m_zoneRadiusSqrd );
    ofVec2f pos    = m_position + m_owner->m_position;
    
    ofSetColor( 255, 255, 255, 128 );
    ofDrawCircle( pos, zoneRadius );
    
    ofSetColor( 255, 255, 0, 128 );
    ofDrawCircle( pos, zoneRadius * m_owner->m_highThresh );
    
    ofSetColor( 255, 0, 255, 128 );
    ofDrawCircle( pos, zoneRadius * m_owner->m_lowThresh );
    
    // TODO: draw id text - if needed
    //ofSetColor( 255, 255, 255, 25 );
    //sgui::SimpleGUI::textureFont->drawString( boost::lexical_cast< std::string >( m_id ), pos + ofVec2f( 5.0f, 5.0f ) );
}

void Particle::limitSpeed()
{
    float vLengthSqrd = m_velocity.lengthSquared();
    
    if ( vLengthSqrd > m_maxSpeedSquared )
    {
        m_velocity = m_direction * m_maxSpeedSquared;
        
    } 
    else if ( vLengthSqrd < m_minSpeedSquared )
    {
        m_velocity = m_direction * m_minSpeedSquared;
    }
}

