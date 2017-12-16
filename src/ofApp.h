#pragma once

#include "ofMain.h"
#include "ofxAudioAnalyzer.h"
#include "ofxDatGui.h"
#include "ofxProcessFFT.h"
#include "ofxPostProcessing.h"

#include "ParticleEmitter.h"

#include <string>
#include <list>

class ofApp : public ofBaseApp{
    
public:
    ofApp( std::list< std::string >& _args );
    // main functions
    void setup( void );
    void update( void );
    void draw( void );
    
    // mouse stuff
    void keyPressed(    int key                  );
    void keyReleased(   int key                  );
    void mouseMoved(    int x, int y             );
    void mouseDragged(  int x, int y, int button );
    void mousePressed(  int x, int y, int button );
    void mouseReleased( int x, int y, int button );
    void mouseEntered(  int x, int y             );
    void mouseExited(   int x, int y             );
    
    // event functions
    void exit( ofEventArgs & args );
    void windowResized( int w, int h    );
    void dragEvent( ofDragInfo dragInfo );
    void gotMessage( ofMessage msg      );
    
    void onToggleStrobe( ofxDatGuiToggleEvent e );
    void onToggleRGBShiftPass( ofxDatGuiToggleEvent e );
    void onToggleNoiseWarpPass( ofxDatGuiToggleEvent e );
    void onToggleBloomPass( ofxDatGuiToggleEvent e );
    void onToggleZoomBlurPass( ofxDatGuiToggleEvent e );
    // POST PROCESSING STUFF <<
    void onDropdownEvent( ofxDatGuiDropdownEvent e );
    
    // gui buttons
    void openImageCallBack( ofxDatGuiButtonEvent e );
    bool openImage( void );
    void nextImageCallBack( ofxDatGuiButtonEvent e );
    bool nextImage( void );
    
    // misc routines
    void updateOutputArea( ofVec2f& _imageSize );
    void setImage( std::string _path );
    
    // properties
    ofPixels*                   m_surface;
    ofImage                     m_texture;
    ofRectangle                 m_outputArea;
    ParticleEmitter             m_particleEmitter;
    ofFbo                       m_frameBufferObject;
    std::list< std::string >    m_files;
    float                       m_cycleImageEvery;
    
    ofxDatGui*                  m_gui;
    ofxDatGui*                  m_audio;
    ofxDatGui*                  m_guiHelp;
    ofxDatGuiButton*            m_openImageButton;
    ofxDatGuiButton*            m_nextImageButton;
    ofxDatGuiLabel*             m_currentImageLabel;
    ofxDatGuiFolder*            m_mainPanel;
    ofxDatGuiFolder*            m_helpPanel;
    ofxDatGuiFRM*               m_FPSPanel;
    
private:
    void changeImage( void );
    std::string                 m_imageToSet;
    
    
    float                       m_lastTime;
    float                       m_currentTime;
    float                       m_cycleCounter;
    
    float*                      m_lowPointer;
    float*                      m_midPointer;
    float*                      m_highPointer;
    
    std::pair< int, int >       m_lowFftRange;
    std::pair< int, int >       m_midFftRange;
    std::pair< int, int >       m_highFftRange;
    
    ProcessFFT                  m_fft;
    ofxAudioAnalyzer            m_audioAnalyzer;
    
    // AUDIO STUFF >>
    ofSoundBuffer               m_soundBuffer;
    
    float                       m_smoothing;
    float                       m_rms;
    float                       m_power;
    float                       m_pitchFreq;
    float                       m_pitchFreqNorm;
    float                       m_pitchConf;
    float                       m_pitchSalience;
    float                       m_hfc;
    float                       m_hfcNorm;
    float                       m_specComp;
    float                       m_specCompNorm;
    float                       m_centroid;
    float                       m_centroidNorm;
    float                       m_inharmonicity;
    float                       m_dissonance;
    float                       m_rollOff;
    float                       m_rollOffNorm;
    float                       m_oddToEven;
    float                       m_oddToEvenNorm;
    float                       m_strongPeak;
    float                       m_strongPeakNorm;
    float                       m_strongDecay;
    float                       m_strongDecayNorm;
    float                       m_danceability;
    float                       m_danceabilityNorm;
    
    std::vector< float >        m_spectrum;
    std::vector< float >        m_melBands;
    std::vector< float >        m_mfcc;
    std::vector< float >        m_hpcp;
    
    std::vector< float >        m_tristimulus;
    
    bool                        m_isOnset;
    bool                        m_strobe;
    // AUDIO STUFF <<
    
    static bool                 s_debugFFt;
    static std::vector< std::string >  s_particleBehaviors;
    
    // POST PROCESSING STUFF >>
    ofxPostProcessing                   m_post;
    std::shared_ptr< RGBShiftPass >     m_rgbShift;
    std::shared_ptr< NoiseWarpPass >    m_noiseWrap;
    std::shared_ptr< BloomPass >        m_bloomPass;
    std::shared_ptr< ZoomBlurPass >     m_zoomBlurPass;
    // POST PROCESSING STUFF <<


};
