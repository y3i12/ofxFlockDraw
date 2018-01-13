#include "ofMain.h"
#include "ofApp.h"
#include <list>
#include <string>

//========================================================================
int main( int argc, char** argv ){

    std::list< std::string > theArgs;
    for ( auto i = 0; i < argc; ++i ) {
        theArgs.push_back( std::string( argv[ i ] ) );
    }
    
	ofGLFWWindowSettings windowSettings;
#ifdef USE_PROGRAMMABLE_GL
    windowSettings.setGLVersion( 4, 1 );
#endif
    windowSettings.width        = 1680;
    windowSettings.height       = 1050;
    windowSettings.windowMode   = OF_FULLSCREEN; // OF_WINDOW;
    //windowSettings.windowMode   =  OF_WINDOW;
    
    ofCreateWindow( windowSettings );
    
	ofRunApp( new ofApp( theArgs ) );

}
