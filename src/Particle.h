#if !defined __PARTICLE_H__
#define __PARTICLE_H__

#include "ofMain.h"

class ParticleEmitter;

class Particle
{    
public:
    Particle( ParticleEmitter* _owner, ofVec2f& _position, ofVec2f& _direction );
    
    static void init( void );
    
    virtual ~Particle( void );
    
    void applyInstantForce( ofVec2f _force );
    void applyForce( ofVec2f _force );
    virtual void update( float _currentTime, float _delta, float _sizeFactor );
    void updateTimer( float _delta );
    virtual void draw( void );
    virtual void debugDraw( void );
    
    ofVec2f&    position() { return m_position; }
    
protected:
    void         limitSpeed();
    
    
public:
    // Particle attributes -> needs to be mapped to a texture
    
    // texture 1: will contain attributes updated by the particle shader itself
    ofVec2f             m_position;             // t1.xy
    ofVec2f             m_velocity;             // t1.zu
    
    // texture 2: will other attributes controlled by the particle shader
    float               m_lifeTime;             // t2.x
    float               m_lifeTimeLeft;         // t2.y
    float               m_maxSpeedSquared;      // t2.z
    float               m_minSpeedSquared;      // t2.u
    
    // texture 3: will contain input forces modificable from outside
    ofVec2f             m_acceleration;         // t.xy
    ofVec2f             m_instantAcceleration;  // t.zu -> is this really needed?
    ofVec2f             m_oldPosition;          // this will not be needed, the old position can be used before actually commiting the vertex
    
    ofColor             m_color;                // will be acquired from image texture
    unsigned char       m_alpha;                // based on time, can be computed
    
    
    
    bool                m_flockLeader;          // Thinking on removing follow the lead - it is worthless
    
    ofPixels*&          m_referenceSurface;
    
    ParticleEmitter*    m_owner;
    
    int                 m_group;                // is this needed? groups can be different particle emitters
    
public:
    // all of this go as parameters to the shader
    static ofParameter< float >     s_maxRadius;
    static ofParameter< float >     s_particleSizeRatio;
    static ofParameter< float >     s_particleSpeedRatio;
    static ofParameter< float >     s_friction;
    static ofParameter< float >     s_dampness;
    static ofParameter< float >     s_colorRedirection;
    static ofParameterGroup         s_particleParameters;
    
private:
    // The temporary parameters will all be discarded
    // temporary variables to avoid construction every update
    ofColor             t_color;
    ofVec2f             t_tempDir;
    float               t_angle;
    ofVec2f             t_nextPos[ 3 ];
    float               t_l[ 3 ];
    ofColor             t_currentColor;
    ofColor             t_c;
    
    size_t              m_id;
    static size_t       s_idGenerator;
    
    
};

#endif // __PARTICLE_H__
