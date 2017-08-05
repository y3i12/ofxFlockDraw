#include "ofApp.h"

#define SGUI_CONFIG_FILE_EXT "cfg"
#define FRAMERATE            60.0f

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include <sstream>
#include <math.h>

#define DEBUG_DRAW

ofApp::ofApp( std::list< std::string >& _args ) :
    m_particleEmitter( m_surface )
{
    ofSetupOpenGL( 1024, 768, OF_WINDOW );			// <-------- setup the GL context
    _args.pop_front();
    for ( auto file : _args )
    {
        m_files.push_back( file );
    }
}

//--------------------------------------------------------------
void ofApp::setup()
{
    // Initialize OFX
    ofRestoreWorkingDirectoryToDefault();// little trick to make debugging easier
    
    ofBackground( 0, 0, 0 );
    ofSetFrameRate( 60 );
    ofPoint displaySz = ofGetWindowSize();

    m_frameBufferObject.allocate( displaySz.x, displaySz.y, GL_RGBA32F_ARB);
    
    // config vars
    m_cycleImageEvery = 0.0;
    
    // buffer for trails
    m_frameBufferObject.begin();
    ofClear( 0, 0, 0, 0 );
    m_frameBufferObject.end();
    
    // emitter
    m_particleEmitter.m_maxLifeTime        = 0.0;
    m_particleEmitter.m_minLifeTime        = 10.0;
    
    // GUI
    m_gui             = new ofxDatGui( ofxDatGuiAnchor::TOP_LEFT  );
    m_guiHelp         = new ofxDatGui( ofxDatGuiAnchor::TOP_RIGHT );
    
    //m_gui->lightColor = ci::ColorA( 1, 1, 0, 1 );
    //m_gui->textFont   = ci::Font( "Consolas", 12 );
    //m_mainPanel = m_gui->addPanel();
    
    // general settings
    m_gui->addLabel( "General Settings" );
    m_gui->addSlider( "Pic. Cycle Time",    m_cycleImageEvery,               3.0f, 120.0f, 15.0f );
    m_gui->addSlider( "Particle Size",      Particle::s_particleSizeRatio,  0.25f,   3.0f,  1.0f );
    m_gui->addSlider( "Particle Speed",     Particle::s_particleSpeedRatio,  0.2f,   3.0f,  1.0f );
    m_gui->addSlider( "Dampness",           Particle::s_dampness,           0.01f,  0.99f,  0.9f );
    m_gui->addSlider( "Color Guidance",     Particle::s_colorRedirection,    0.0f, 360.0f, 90.0f );
    
    m_gui->addSlider( "#Particles/Group",   ParticleEmitter::s_particlesPerGroup,  50,    5000,   500 );
    m_gui->addSlider( "#Groups",            ParticleEmitter::s_particleGroups,      1,      20,     5 );
    
    m_gui->addBreak()->setHeight( 20.0f );
    m_gui->addLabel( "Flocking Settings" );
    
    m_gui->addSlider( "Repel Str.",         m_particleEmitter.m_repelStrength,       0.000f,     10.0f,   2.0f );
    m_gui->addSlider( "Align Str.",         m_particleEmitter.m_alignStrength,       0.000f,     10.0f,   2.0f );
    m_gui->addSlider( "Att. Str.",          m_particleEmitter.m_attractStrength,     0.000f,     10.0f,   1.0f );
    m_gui->addSlider( "Area Size",          m_particleEmitter.m_zoneRadiusSqrd,      625.0f, 10000.0f, 5625.0f ),
    m_gui->addSlider( "Repel Area",         m_particleEmitter.m_lowThresh,             0.0f,     1.0f,  0.125f );
    m_gui->addSlider( "Align Area",         m_particleEmitter.m_highThresh,            0.0f,     1.0f,   0.65f );
    
    m_gui->addBreak()->setHeight( 20.0f );
    
    m_openImageButton = m_gui->addButton( "Open Image" );
    m_openImageButton->onButtonEvent( this, &ofApp::openImageCallBack );
    
    m_nextImageButton = m_gui->addButton( "Next Image" );
    m_nextImageButton->onButtonEvent( this, &ofApp::nextImageCallBack );
    
    m_gui->addBreak()->setHeight( 20.0f );
    m_gui->addFRM();
    
    // some info
    m_gui->addBreak()->setHeight( 10.0f );
    m_gui->addLabel( "Current Image:" );
    m_currentImageLabel = m_gui->addLabel( "" );
    
    // deserved credits
    m_gui->addBreak()->setHeight( 10.0f );
    m_gui->addLabel( "y3i12: Yuri Ivatchkovitch" );
    m_gui->addLabel( "http://y3i12.com/"  );
    
    // Help!
    m_guiHelp         = new ofxDatGui( ofxDatGuiAnchor::TOP_RIGHT );
    
    /*
     int x = gui->getPosition().x + gui->getWidth() + 40;
     l1 = new ofxDatGuiLabel("sliders from of_parameters");
     l1->setPosition(x, gui->getPosition().y);
*/
    
    m_guiHelp->addLabel( "Quick Help:"              );
    m_guiHelp->addLabel( "F1  to show/hide help"    );
    m_guiHelp->addLabel( "'h' to show/hide GUI"     );
    m_guiHelp->addLabel( "'s' to save config"       );
    m_guiHelp->addLabel( "'l' to load config"       );
    m_guiHelp->addLabel( "'o' to open image"        );
    m_guiHelp->addLabel( "'c' to start/end capture" );
    m_guiHelp->addLabel( "'f' to hide/show fps"     );
    m_guiHelp->addLabel( "SPACE to skip image"      );
    m_guiHelp->addLabel( "ESC to quit"              );
    
    m_gui->addBreak()->setHeight( 20.0f );
    
    m_guiHelp->addLabel( "Drag multiple files"    );
    m_guiHelp->addLabel( "to slideshow!"          );

    
    // remove invalid paths
    for ( auto itr = m_files.begin(); itr != m_files.end(); ++itr ) {
        
        ofFile file( *itr );
        if ( !file.exists() )
        {
            auto t_itr = itr;
            
            ++t_itr;
            m_files.erase( itr );
            itr = --t_itr;
        }
    }
    
    // load images passed via args
    if ( m_files.size() > 1 )
    {
        m_cycleCounter = 0;
    }
    else
    {
        if ( m_files.size() == 0 )
        {
            m_files.push_back( "heaven.png" );
        }
        setImage( m_files.front(), m_currentTime );
        m_cycleCounter  = -1.0;
    }
    
    // mark the time to test counters
    m_lastTime = ofGetElapsedTimef();
}

//--------------------------------------------------------------
void ofApp::update()
{
    float delta   = 0.0;
    m_currentTime = ofGetElapsedTimef();
    delta         = m_currentTime - m_lastTime;
    
    if ( m_cycleCounter != -1.0 )
    {
        m_cycleCounter += delta;
        
        if ( m_cycleCounter >= m_cycleImageEvery && m_files.size() > 1 )
        {
            m_cycleCounter -= m_cycleImageEvery;
            
            std::string aPath = m_files.front();
            m_files.pop_front( );
            m_files.push_back( aPath );
            
            setImage( aPath, m_currentTime );
        }
    }
    
    m_particleEmitter.update( m_currentTime, delta );
    
    m_lastTime = m_currentTime;
}

//--------------------------------------------------------------
void ofApp::draw()
{
    ofPoint displaySz = ofGetWindowSize();
    
    // do the drawing =D
    m_frameBufferObject.begin();
        ofFill();
        ofSetColor( 0, 0, 0, 1 );
        ofDrawRectangle( 0, 0, displaySz.x, displaySz.y );
    
        ofSetColor( 0, 0, 0, 255 );
        /*
        blendMode = OF_BLENDMODE_ALPHA;
        blendMode = OF_BLENDMODE_ADD;
        blendMode = OF_BLENDMODE_MULTIPLY;
        blendMode = OF_BLENDMODE_SUBTRACT;
        blendMode = OF_BLENDMODE_SCREEN;
         */
        //ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        m_particleEmitter.draw();
        //ofDisableBlendMode();
        m_frameBufferObject.end();
    
    // save what happened to the framebuffer
    ofSetColor( 255, 255, 255 );
    m_frameBufferObject.draw( 0, 0 );
    
    if ( ParticleEmitter::s_debugDraw )
    {
        m_particleEmitter.debugDraw();
    }
    
    // draw the UI
    m_gui->draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch( key )
    {
#if defined DEBUG_DRAW
        case 'd':
        {
            ParticleEmitter::s_debugDraw = !ParticleEmitter::s_debugDraw;
        }
        break; //prints values of all the controls to the console
#endif
        case 'l':
        {
            std::vector< std::string > theExtensions;
            theExtensions.push_back( SGUI_CONFIG_FILE_EXT );
            
            ofFileDialogResult openFileResult= ofSystemLoadDialog( "Load settings" );
            
            //Check if the user opened a file
            if ( openFileResult.bSuccess )
            {
                ofLogVerbose("User selected a file");
                
                // We have a file, check it and process it
                std::string aPath = openFileResult.getPath();
                
                ofFile file( aPath );
                
                if ( file.exists() && file.getExtension() == SGUI_CONFIG_FILE_EXT )
                {
                    ofSystemAlertDialog( "to be implemented" );
                    // TODO
                    // m_gui->load( aPath );
                }
                else
                {
                    ofSystemAlertDialog( "Invalid file" );
                }
            }
        }
        break;
            
            
        case 's':
        {
            ofFileDialogResult saveFileResult = ofSystemSaveDialog( "*.cfg", "Save settings");
            
            if ( saveFileResult.bSuccess )
            {
                ofFile file( saveFileResult.filePath );
                
                if ( file.getExtension() != SGUI_CONFIG_FILE_EXT )
                {
                    ofSystemAlertDialog( "Invalid extension" );
                }
                else
                {
                    ofSystemAlertDialog( "to be implemented" );
                    // TODO
                    //m_gui->save( saveFileResult.filePath );
                }
            }
        }
        break;
            
        case 'h':
        {
            m_gui->setVisible( !m_gui->getVisible() );
        }
        break;
            
        case 'o':
        {
            openImage();
        }
        break;
            
        case OF_KEY_ESC:
        {
            m_particleEmitter.killAll();
            ofExit();
        }
            break;
            
        case OF_KEY_F1:
        {
            m_guiHelp->setVisible( !m_guiHelp->getVisible() );
        }
            break;
            
        case ' ':
        {
            if ( m_files.size() == 1 )
            {
                setImage( m_files.front(), m_currentTime );
            }
            else
            {
                nextImage();
            }
        }
            break;
    }
}

void ofApp::exit( ofEventArgs & args )
{
    m_particleEmitter.killAll();
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 
    /* Is this here?
    m_files = _event.getFiles();
    
    setImage( m_files.front(), m_currentTime );
    
    if ( m_files.size() > 1 )
    {
        m_cycleCounter = 0;
    }
     */
}

////////////////////////////////////////////////////////////////////////////////

void ofApp::openImageCallBack( ofxDatGuiButtonEvent e )
{
    return openImage();
}

bool ofApp::openImage( void )
{
    m_files.clear();
    /*
     Use http://openframeworks.cc/documentation/utils/ofDirectory/ to scan dirs
     BMP files [reading, writing]
     Dr. Halo CUT files [reading] *
     DDS files [reading]
     EXR files [reading, writing]
     Raw Fax G3 files [reading]
     GIF files [reading, writing]
     HDR files [reading, writing]
     ICO files [reading, writing]
     IFF files [reading]
     JBIG files [reading, writing] **
     JNG files [reading, writing]
     JPEG/JIF files [reading, writing]
     JPEG-2000 File Format [reading, writing]
     JPEG-2000 codestream [reading, writing]
     JPEG-XR files [reading, writing]
     KOALA files [reading]
     Kodak PhotoCD files [reading]
     MNG files [reading]
     PCX files [reading]
     PBM/PGM/PPM files [reading, writing]
     PFM files [reading, writing]
     PNG files [reading, writing]
     Macintosh PICT files [reading]
     Photoshop PSD files [reading]
     RAW camera files [reading]
     Sun RAS files [reading]
     SGI files [reading]
     TARGA files [reading, writing]
     TIFF files [reading, writing]
     WBMP files [reading, writing]
     WebP files [reading, writing]
     XBM files [reading]
     XPM files [reading, writing]
     */
    ofFileDialogResult fileOpenResult = ofSystemLoadDialog( "Load an image" );
    
    if ( fileOpenResult.bSuccess )
    {
        ofFile file(fileOpenResult.getPath() );
        if ( file.exists() )
        {
            setImage( file.getAbsolutePath(), m_currentTime );
        }
        
        m_cycleCounter  = -1.0;
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

void ofApp::nextImageCallBack( ofxDatGuiButtonEvent e )
{
    return nextImage();
}

bool ofApp::nextImage( void )
{
    m_cycleCounter = m_cycleImageEvery;
    return false;
}

////////////////////////////////////////////////////////////////////////////////

void ofApp::updateOutputArea( ofVec2f& _imageSize )
{
    ofPoint     windowSz    = ofGetWindowSize();
    m_outputArea.x          = windowSz.x / 2 - _imageSize.x / 2;
    m_outputArea.y          = windowSz.y / 2 - _imageSize.y / 2;
    m_outputArea.width      = _imageSize.x;
    m_outputArea.height     = _imageSize.y;
    
    m_particleEmitter.m_position = ofVec2f( static_cast< float >( m_outputArea.x ), static_cast< float >( m_outputArea.y ) );
}

void ofApp::setImage( std::string _path, double _currentTime )
{
    m_particleEmitter.pauseThreads();
    // load the image, resize and set the texture
    m_texture.load( _path );
    ofVec2f   aSize( m_texture.getWidth(), m_texture.getHeight() );
    ofPoint   windowSz    = ofGetWindowSize();
    float     theFactor   = fmin( windowSz.x / aSize.x, windowSz.y / aSize.y );
    
    ofVec2f newSize( aSize.x * theFactor, aSize.y * theFactor );
    m_texture.resize( static_cast< int >( newSize.x ), static_cast< int >( newSize.y ) );
    m_surface = &m_texture.getPixels();
    
    // update  the image name
    m_currentImageLabel->setName( _path );
    
    // update the output area
    updateOutputArea( newSize );
    
    // resets the cycle counter;
    m_cycleCounter = 0.0;
    
    // set the filename in the widget
    ofFile file( _path );
    m_currentImageLabel->setLabel( file.getFileName() );
    
    m_particleEmitter.continueThreads();
}

////////////////////////////////////////////////////////////////////////////////
