#if !defined __PARTICLE_EMITTER_H__
#define __PARTICLE_EMITTER_H__

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>

#include "Particle.h"
#include "ofxFlowTools.h"
#include "ofMain.h"

using namespace flowTools;
class ParticleEmitter
{
public:
    enum UpdateType {
        kFunction               = 1 << 0,
        kFlocking               = 1 << 1,
        kFunctionAndFlocking    = kFunction | kFlocking,
        kFollowTheLead          = 1 << 3,
        kOpticalFlow            = 1 << 4,
        kFluidVelocity          = 1 << 5
    };
    
    typedef std::function< float ( float ) > PosFunc;
    
    struct FuncCtl {
        FuncCtl( std::vector< ParticleEmitter::PosFunc >& _fn );
        float operator()( float x );
        void randomize( void );
        void update( float _delta );
        
        std::vector< ParticleEmitter::PosFunc >&     m_fnList;
        size_t                      m_fn[ 2 ];
        
        float                       m_funcTimer;
        float                       m_funcTimeout;
        
        static float                s_minChangeTime;
        static float                s_maxChangeTime;
    };
    
public:
    ParticleEmitter( ofPixels*& _surface );
    virtual ~ParticleEmitter( void );
    
    virtual void draw( void );
    void drawFluidVelocity( void );
    void drawOpticalFlow( void );
    virtual void debugDraw( void );
    virtual void update( float _currentTime, float _delta );
    virtual void updateVideo( bool _isNewFrame, ofBaseDraws& _source );
    
    virtual void pauseThreads( void );
    virtual void continueThreads( void );
    virtual void killAll( void );
    
    std::vector< std::vector< Particle* > > m_particles;
    ofVec2f                     m_position;
    float                       m_maxLifeTime;
    float                       m_minLifeTime;
    
    float                       m_soundLow;
    float                       m_soundMid;
    float                       m_soundHigh;
    
    UpdateType                  m_updateType;
    
    // flocking vars
    float                       m_zoneRadiusSqrd;
    float                       m_repelStrength;
    float                       m_alignStrength;
    float                       m_attractStrength;
    float                       m_lowThresh;
    float                       m_highThresh;
    
    // image related
    ofPixels*&                  m_referenceSurface;
    float                       m_sizeFactor;
    
    static float                s_minParticleLife;
    static float                s_maxParticleLife;
    
    static int                  s_particlesPerGroup;
    static int                  s_particleGroups;
    static bool                 s_debugDraw;
    
    void waitThreadedUpdate( void );
    
private:
    void addParticles( int _group = -1 );
    void startThreadedUpdate( void );
    void threadProcessParticles( size_t _group );
    
    void updateParticles(               float _currentTime, float _delta, std::vector< Particle* >& _particles );
    void updateParticleTiming(          float _currentTime, float _delta, std::vector< Particle* >& _particles );
    void updateParticlesFollowTheLead(  float _currentTime, float _delta, std::vector< Particle* >& _particles );
    void updateParticlesFunctions(      float _currentTime, float _delta, std::vector< Particle* >& _particles );
    void updateParticlesFlocking(       float _currentTime, float _delta, std::vector< Particle* >& _particles );
    void updateParticlesFromSource(     float _currentTime, float _delta, std::vector< Particle* >& _particles, ofTexture& _source );
    void updateParticlesOpticalFlow(    float _currentTime, float _delta, std::vector< Particle* >& _particles );
    void updateParticlesFluidVelocity(  float _currentTime, float _delta, std::vector< Particle* >& _particles );
    
    // Threading stuff
    std::vector< std::thread >  m_threads;          // Thread pool
    std::atomic_bool            m_stop;             // Stop
    std::atomic_bool            m_pause;            // Make the threads to not do their work
    std::atomic_size_t          m_processing;       // Number of threads processing
    
    std::mutex                  m_updateLock;       // Controls the thread sync
    std::condition_variable     m_conditionVar;     // Thread control
    
    // Time stuff
    float                       m_currentTime;
    float                       m_delta;
    
    float                       m_updateFlockEvery;
    float                       m_updateFlockTimer;
    float                       m_lastFlockUpdateTime;
    
    // Counter stuff
    int                         m_particlesPerGroup;
    int                         m_particleGroups;
    
    // Function control stuff
    FuncCtl                     m_velocityAudioFunc;
    FuncCtl                     m_xMathFunc;
    FuncCtl                     m_yMathFunc;

    std::vector< PosFunc >      m_mathFn;
    std::vector< PosFunc >      m_audioFn;
    
    // Optical flow and fluid stuff
    int                         m_flowWidth;
    int                         m_flowHeight;
    
    ftOpticalFlow               m_opticalFlow;
    ftFluidSimulation           m_fluidSimulation;
    ftVelocityMask              m_velocityMask;

    ftDisplayScalar             m_scalarDisplay;
    ftVelocityField             m_velocityField;
    
    ftFbo                       m_ftBo;
};

#endif //__PARTICLE_EMITTER_H__
