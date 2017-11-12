#pragma once

#include "ofMain.h"
#include "ofxDatGui.h"
#include "ofxProcessFFT.h"

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
    
    // gui buttons
    void openImageCallBack( ofxDatGuiButtonEvent e );
    bool openImage( void );
    void nextImageCallBack( ofxDatGuiButtonEvent e );
    bool nextImage( void );
    
    // misc routines
    void updateOutputArea( ofVec2f& _imageSize );
    void setImage( std::string _path, double _currentTime = 0.0 );
    
    // properties
    ofPixels*                   m_surface;
    ofImage                     m_texture;
    ofRectangle                 m_outputArea;
    ParticleEmitter             m_particleEmitter;
    ofFbo                       m_frameBufferObject;
    std::list< std::string >    m_files;
    float                       m_cycleImageEvery;
    
    ofxDatGui*                  m_gui;
    ofxDatGui*                  m_guiHelp;
    ofxDatGuiButton*            m_openImageButton;
    ofxDatGuiButton*            m_nextImageButton;
    ofxDatGuiLabel*             m_currentImageLabel;
    ofxDatGuiFolder*            m_mainPanel;
    ofxDatGuiFolder*            m_helpPanel;
    ofxDatGuiFRM*               m_FPSPanel;
    
private:
    float                       m_lastTime;
    float                       m_currentTime;
    float                       m_cycleCounter;
    
    float*                      m_lowPointer;
    float*                      m_midPointer;
    float*                      m_highPointer;
    
    ProcessFFT                  m_fft;
    
    static bool                 s_debugFFt;

};
