#pragma once

#include "ofMain.h"
#include "towerPatterns.h"
#include "XBee.h"

class testApp : public ofBaseApp{
public:
    bool test = false;
    void setup();
    void initializePattern();
    void setupRadio();
    void update();
    void draw();
    void drawGPSdata();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    void processIncoming();
    void processRXResponse();
    void sendHeartbeat();
    
    ofTrueTypeFont font;
    string patternDisplayName(furSwarmPatterns* tower);
    string patternName(int patternId);
#ifdef FS_TOWER
    vector <towerPatterns*> platforms;
#else
    vector <furSwarmPatterns*> platforms;
#endif
    list <uint8_t*> messages;
    uint8_t data[7];
    
    //  XBee Attributes & Commands
    string portName;
    ofSerial	serial;
    XBee        xbee;
    uint8_t     lastDataLength = 0;
};
