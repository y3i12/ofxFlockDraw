#include "Particle.h"
#include "ParticleEmitter.h"

#include <cmath>

//#define DEG2RAD( x ) ( ( x ) * 0.017453292519943295769236907684886f )
//#define RAD2DEG( x ) ( ( x ) * 57.2958f )
#define HPI ( PI / 2 )
#define LUMINANCE( r, g, b ) ( 0.299f * ( r ) + 0.587f * ( g ) + 0.114f * ( b ) )

bool WRAP( ofVec2f& p, ofVec2f& w )
{
    bool r = false;
    if ( p.x < 0.0f )
    {
        p.x = w.x - fmod( fabs( p.x ), w.x );
        r = true;
    }
    else if ( p.x >= w.x )
    {
        p.x  = fmod( p.x, w.x );
        r = true;
    }
    
    if ( p.y < 0.0f )
    {
        p.y = w.y - fmod( fabs( p.y ), w.y );
        r = true;
    }
    else if ( p.y >= w.y )
    {
        p.y  = fmod( p.y, w.y );
        r = true;
    }
    
    return r;
}

ofParameter< float >    Particle::s_maxRadius{           "Max Radius",        0.25f, 0.001f,   1.0f };
ofParameter< float >    Particle::s_particleSizeRatio{   "Size Ratio",        1.0f,  0.001f,   1.0f };
ofParameter< float >    Particle::s_particleSpeedRatio{  "Speed Ratio",       1.0f,    0.0f,  10.0f };
ofParameter< float >    Particle::s_friction{            "Friction",          0.9f,    0.0f,   1.0f };
ofParameter< float >    Particle::s_dampness{            "Dampness",          0.9f,    0.0f,   1.0f };
ofParameter< float >    Particle::s_colorRedirection{    "Color Guidance",   90.0f,    0.0f, 360.0f };
ofParameterGroup        Particle::s_particleParameters;
size_t                  Particle::s_idGenerator        = 0;

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
    m_flocked( false ),
    m_owner( _owner ),
    m_group( -1 ),
    m_id( s_idGenerator++ ),
    m_referenceSurface( _owner->m_referenceSurface )
{
    
}
void Particle::init( void )
{
    if ( 0 == s_particleParameters.size() )
    {
        s_particleParameters.setName( "Particle" );
        s_particleParameters.add( s_maxRadius, s_particleSizeRatio, s_particleSpeedRatio, s_friction, s_dampness, s_colorRedirection );
    }
}

Particle::~Particle( void )
{
}

void Particle::applyInstantForce( ofVec2f _force )
{
    m_instantAcceleration  += _force;
    m_direction             = ( m_velocity + m_acceleration + m_instantAcceleration ).getNormalized();
}

void Particle::applyForce( ofVec2f _force )
{
    m_acceleration += _force;
    m_direction     = ( m_velocity + m_acceleration + m_instantAcceleration ).getNormalized();
}

void Particle::update( float _currentTime, float _delta, float _sizeFactor )
{
    if ( m_referenceSurface )
    {
        m_oldPosition = m_position;
        
        // capping to avoid errors
        if ( std::isnan( m_velocity.x ) || fabs( m_velocity.x ) > 1000.0f ||
            std::isnan( m_velocity.y ) || fabs( m_velocity.y ) > 1000.0f )
        {
            m_velocity.normalize();
        }
        
        
        // update the speed
        m_velocity     += m_acceleration + m_instantAcceleration;
        m_instantAcceleration.set( 0.0f, 0.0f );
        
        // capping to avoid errors
        if ( std::isnan( m_velocity.x ) ) m_velocity.x = 0.0f;
        if ( std::isnan( m_velocity.y ) ) m_velocity.y = 0.0f;
        if ( std::isnan( m_position.x ) ) m_position.x = 0.0f;
        if ( std::isnan( m_position.y ) ) m_position.y = 0.0f;
        
        // wrap the particle
        ofVec2f wrapSize( m_referenceSurface->getWidth() * _sizeFactor, m_referenceSurface->getHeight() * _sizeFactor );
        
        if ( WRAP( m_position, wrapSize ) )
        {
            m_oldPosition = m_position;
        }
        
        t_tempDir = m_direction * 2.0f;
        t_angle   = 45;
        
        t_currentColor = m_referenceSurface->getColor( static_cast< int >( m_position.x / _sizeFactor ), static_cast< int >( m_position.y / _sizeFactor ) );
        t_color        = t_currentColor;
        m_color        = m_color / 2 + t_color / 2;
        
        t_nextPos[ 0 ] = m_position + t_tempDir;
        t_tempDir.rotate( t_angle );
        t_nextPos[ 1 ] = m_position + t_tempDir;
        t_tempDir.rotate( t_angle * -2.0f );
        t_nextPos[ 2 ] = m_position + t_tempDir;
        
        // image guidance
        for ( int i = 0; i < 3; ++i )
        {
            ofVec2f& pointRef = t_nextPos[ i ];
            ofVec2f colorSource( static_cast< int >( pointRef.x / _sizeFactor ), static_cast< int >( pointRef.y / _sizeFactor ) );
            ofVec2f wrapSizeScaled( m_referenceSurface->getWidth(), m_referenceSurface->getHeight() );
            
            WRAP( colorSource, wrapSizeScaled );
            
            // to guide thru color
            t_c      = t_currentColor - m_referenceSurface->getColor( colorSource.x, colorSource.y );
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
        
        limitSpeed();
        // update the position
        m_position     += m_velocity * _delta * Particle::s_particleSpeedRatio;
        if ( WRAP( m_position, wrapSize ) )
        {
            m_oldPosition = m_position;
        }
        
        m_velocity     -= m_velocity     * ( 1.0f - Particle::s_friction ) * _delta;
        m_acceleration -= m_acceleration * ( 1.0f - Particle::s_dampness ) * _delta;
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
    float radius = ( 1.0f + Particle::s_maxRadius * LUMINANCE( t_color.r, t_color.g, t_color.b ) ) * Particle::s_particleSizeRatio;
    
    ofSetColor( m_color.r, m_color.g, m_color.b, m_alpha );
    //ofFill();
    //ofDrawCircle( m_position + m_owner->m_position, radius );
    ofSetLineWidth( radius );
    ofDrawLine( m_position + m_owner->m_position, m_oldPosition + m_owner->m_position );
}

void Particle::debugDraw( void )
{
    ofNoFill();
    float zoneRadius = m_owner->s_zoneRadius;
    ofVec2f pos    = m_position + m_owner->m_position;
    
    ofSetColor( 255, 255, 255, 128 );
    ofDrawCircle( pos, zoneRadius );
    
    ofSetColor( 255, 255, 0, 128 );
    ofDrawCircle( pos, zoneRadius * m_owner->s_highThresh );
    
    ofSetColor( 255, 0, 255, 128 );
    ofDrawCircle( pos, zoneRadius * m_owner->s_lowThresh );
    
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

