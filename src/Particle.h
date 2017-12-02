#if !defined __PARTICLE_H__
#define __PARTICLE_H__

#include "ofMain.h"

class ParticleEmitter;

class Particle
{
public:
    struct PointAccessFunctor
    {
        static inline ofVec2f& position( Particle* p )
        {
            return p->m_position;
        }
        
        static inline ofVec2f& stable_position( Particle* p )
        {
            return p->m_stablePosition;
        }
    };
    
public:
    Particle( ParticleEmitter* _owner, ofVec2f& _position, ofVec2f& _direction );
    
    virtual ~Particle( void );
    
    void applyForce( ofVec2f _force, bool _limit = true );
    virtual void update( float _currentTime, float _delta );
    void updateTimer( float _delta );
    virtual void draw( void );
    virtual void debugDraw( void );
    
    ofVec2f&    position() { return m_position; }
    
protected:
    void         limitSpeed();
    
    
public:
    ofVec2f             m_position;
    ofVec2f             m_stablePosition;
    ofVec2f             m_direction;
    ofVec2f             m_velocity;
    ofVec2f             m_acceleration;
    
    ofColor             m_color;
    unsigned char       m_alpha;
    
    float               m_maxSpeedSquared;
    float               m_minSpeedSquared;
    
    float               m_lifeTime;
    float               m_lifeTimeLeft;
    
    ofPixels*&          m_referenceSurface;
    
    ParticleEmitter*    m_owner;
    
    int                 m_group;
    
public:
    static float        s_maxRadius;
    static float        s_particleSizeRatio;
    static float        s_particleSpeedRatio;
    static float        s_dampness;
    static float        s_colorRedirection;
    
private:
    // temporary variables to avoid construction every update
    // ofRectangle         t_sourceArea;
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
