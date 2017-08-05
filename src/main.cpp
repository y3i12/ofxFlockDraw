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
    
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp( new ofApp( theArgs ) );

}
