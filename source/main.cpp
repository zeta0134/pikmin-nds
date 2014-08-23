/*---------------------------------------------------------------------------------

    $Id: main.cpp,v 1.13 2008-12-02 20:21:20 dovoto Exp $

    Simple console print demo
    -- dovoto


---------------------------------------------------------------------------------*/
#include <nds.h>
#include <stdio.h>

#include "MultipassEngine.h"
#include "RedPikmin.h"
#include "YellowPikmin.h"
#include "Captain.h"

volatile int frame = 0;

#define TEST_PIKMIN 10

MultipassEngine engine;
RedPikmin red_pikmin[TEST_PIKMIN];
YellowPikmin yellow_pikmin[TEST_PIKMIN];
Captain captain[TEST_PIKMIN];

using namespace std;

//---------------------------------------------------------------------------------
void init() {
//---------------------------------------------------------------------------------
    consoleDemoInit();

    printf("Multipass Engine Demo\n");
    
    //let's get the top screen set up for 3D
    videoSetMode(MODE_0_3D);
                                                         
    // initialize gl
    glInit();
    
    // enable GL features
    glEnable(GL_TEXTURE_2D);
    
    // setup the rear plane
    glClearColor(0,10,0,31);
    glClearDepth(0x7FFF); //TODO: Play with this maybe? This might be why we were getting clipping at the back plane before.
    
    //The entire screen is our plaything
    glViewport(0,0,255,191);
    
    //initialize us to identity; we'll overwrite most of these settings in the engine
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    gluPerspective(70, 256.0 / 192.0, 0.1, 4096);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    gluLookAt(  0.0, 0.0, 10.0,     //camera possition 
                0.0, 0.0, 0.0,      //look at
                0.0, 1.0, 0.0);     //up
    
    //Setup default lights; these may be overridden later
    glLight(0, RGB15(31,31,31) , floattov10(-0.40), floattov10(0.32), floattov10(0.27));
    glLight(1, RGB15(31,31,31) , floattov10(0.32), floattov10(0.32), floattov10(0.32));
    //glLight(2, RGB15(0,31,0) ,   floattov10(-1.0), 0,                  0);
    //glLight(3, RGB15(0,0,31) ,   floattov10(1.0) - 1,  0,                  0);
    
    //setup demo pikmin
    for (int i = 0; i < TEST_PIKMIN; i++) {
        //red_pikmin[i].setPosition({-2.5,0,i * -2.5});
        //engine.addEntity(&red_pikmin[i]);
        
        //yellow_pikmin[i].setPosition({-2.5,0,i * -2.5});
        //engine.addEntity(&yellow_pikmin[i]);
        
        //captain[i].setPosition({-5,0,i * -5});
        //engine.addEntity(&captain[i]);
    }
    
    captain[0].setPosition({0,0,0});
    engine.addEntity(&captain[0]);
    
    glPushMatrix();
}

//---------------------------------------------------------------------------------
void gameloop() {
//---------------------------------------------------------------------------------
    //Example debug code; remove later?
    frame++;
    
    touchPosition touchXY;
    touchRead(&touchXY);

    // print at using ansi escape sequence \x1b[line;columnH 
    //printf("\x1b[10;0HFrame = %d",frame);
    //printf("\x1b[16;0HTouch x = %04X, %04X\n", touchXY.rawx, touchXY.px);
    //printf("Touch y = %04X, %04X\n", touchXY.rawy, touchXY.py);
    
    //Run the game.
    engine.update();
    engine.draw();
}
    
//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------
    init();
    
    
    while(1) {
        gameloop();
    }

    return 0;
}
