#if !defined __PARTICLE_EMITTER_H__
#define __PARTICLE_EMITTER_H__

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>

#include "Particle.h"
#include "ofMain.h"

class ParticleEmitter
{
public:
    ParticleEmitter( ofPixels*& _surface );
    virtual ~ParticleEmitter( void );
    
    virtual void draw( void );
    virtual void debugDraw( void );
    virtual void update( float _currentTime, float _delta );
    
    virtual void pauseThreads( void );
    virtual void continueThreads( void );
    virtual void killAll( void );
    
    std::vector< std::vector< Particle* > > m_particles;
    ofVec2f                     m_position;
    float                       m_maxLifeTime;
    float                       m_minLifeTime;
    
    
    
    
    // flocking vars
    float                       m_zoneRadiusSqrd;
    float                       m_repelStrength;
    float                       m_alignStrength;
    float                       m_attractStrength;
    float                       m_lowThresh;
    float                       m_highThresh;
    
    ofPixels*&                  m_referenceSurface;
    
    static int                  s_particlesPerGroup;
    static int                  s_particleGroups;
    static bool                 s_debugDraw;
    
private:
    void addParticles( int _aumont, int _group = -1 );
    void startThreadedUpdate( void );
    void waitThreadedUpdate( void );
    void threadProcessParticles( size_t _group );
    void updateParticles( float _currentTime, float _delta, std::vector< Particle* >& _particles );
    
    // Threading stuff
    std::vector< std::thread >  m_threads;          // Thread pool
    bool                        m_stop;             // Stop
    bool                        m_pause;            // Make the threads to not do their work
    size_t                      m_processing;       // Number of threads processing
    
    std::mutex                  m_updateLock;       // Controls the thread sync
    std::condition_variable     m_startCondition;   // When threads need too start
    std::condition_variable     m_doneCondition;    // When threads are finished
    
    float                       m_currentTime;
    float                       m_delta;
    
    float                       m_updateFlockEvery;
    float                       m_updateFlockTimer;
    float                       m_lastFlockUpdateTime;
    
    int                         m_particlesPerGroup;
    int                         m_particleGroups;
};

#endif //__PARTICLE_EMITTER_H__
