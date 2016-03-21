#include "testApp.h"
#include "towerPatterns.h"
#include "furSwarmPatternConst.h"
#include "embeddedInterface.h"

// System configuration
#define versionId 0x14
const bool legacyPro = false;
#ifdef FS_VEST
const uint8_t memberType = 0x01; // Vest
#elif FS_HAT
const uint8_t memberType = 0x02; // Hat
#elif FS_TOWER
const uint8_t memberType = 0x03; // Tower
#elif FS_TOWN_CENTER
const uint8_t memberType = 0x04; // Town Center
#endif

// Heartbeat message layout
uint8_t heartbeatPayload[] = {
    0x01,       // Byte 0: Message Type ID (1 byte)
    versionId,  // Byte 1: Version ID (1 byte)
    0,          // Byte 2: Frame location (2 bytes)
    0,
    0,          // Byte 4: Current Pattern
    0,          // Byte 5: Battery Voltage (2 bytes)
    0,
    0,          // Byte 7: Frame Rate (1 byte)
    memberType, // Byte 8: Member type
    0,          // Byte 9: Failed messages (2 bytes)
    0
};
#define frameCountPosition (2)
#define patternPosition (4)
#define voltagePosition (5)
#define frameRatePosition (7)
#define failedMessagePosition (9)

// XBee data
XBee xbee = XBee();
const uint32_t XBeeBaudRate = 57600;
const uint32_t XBeePANID = 0x2014;
XBeeAddress64 addr64 = XBeeAddress64(0x00000000, 0x00000000); // Coord ID free
ZBTxRequest heartbeatMessage = ZBTxRequest(addr64, heartbeatPayload, sizeof(heartbeatPayload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();
ZBRxResponse rxResponse = ZBRxResponse();
unsigned long heartbeatTimestamp = 0;
const unsigned long defaultHeartbeatPeriod = 6000;
const unsigned long hifreqHeartbeatPeriod = 2000;
unsigned long heartbeatPeriod = defaultHeartbeatPeriod;

// FurSwarm member data
uint16_t frameCount = 0;
unsigned long frameRateCount = 0;
int frameStarted = 0;

const int MESSAGE_DISPLAY_LIMIT = 20;

//--------------------------------------------------------------
void testApp::setup(){

    test = false;
    if (test) {
        ofSetFrameRate(5);
    } else {
        ofSetFrameRate(30);
    }
    messages.clear();
	platforms.clear();
#ifdef FS_TOWER_EYE
    int numberOfTowers = 1;
#elif FS_TOWER
    int numberOfTowers = 10;
#elif FS_TOWN_CENTER
    int numberOfTowers = 8;
#else
    int numberOfTowers = 1;
#endif
	for(int i = 0; i < numberOfTowers; i++){
		platforms.push_back(new towerPatterns);
	}
	for(int i = 0; i < platforms.size(); i++){
        platforms[i]->FS_BREATHE_MULTIPLIER = 50.0;
	}
    // Speed, Red, Green, Blue, Intensity
    uint8_t sourceData[] = {FS_ID_ANIMATE_1, 120, 130, 100, 130, 240, 0};
    //uint8_t sourceData[] = {FS_ID_BROKEN, 120, 130, 100, 130, 240, 0};
    //uint8_t sourceData[] = {FS_ID_SEARCHING_EYE, 10, 250, 100, 130, 250, 0};
    //uint8_t sourceData[] = {FS_ID_RAINBOW_CHASE, 120, 130, 100, 130, 240, 0};
    //uint8_t sourceData[] = {FS_ID_PONG, 60, 1, numberOfTowers, 130, 20};
    memcpy (data, sourceData, 7);
    for(int i = 0; i < platforms.size(); i++){
        if (data[0] == FS_ID_PONG) {
            data[2] = i + 1;
        }
        
        platforms[i]->initializePattern(data, 7);
#ifdef FS_TOWER
        platforms[i]->latitude = 100 * sin (2 * PI * i / 10);
        platforms[i]->longitude = 100 * cos (2 * PI * i / 10);
#endif
    }
    font.load("DIN.otf", 10);
    setupRadio();
}

void testApp::setupRadio() {
    serial.listDevices();
    portName = "/dev/tty.usbserial-AH001572";
    serial.setup(portName, XBeeBaudRate);
    xbee.setSerial(serial);
}

//--------------------------------------------------------------
void testApp::update(){
	for(int i = 0; i < platforms.size(); i++){
        platforms[i]->continuePatternDisplay();
        // We need to iterate the display
        // in order to simulate 60Hz due to
        // the macBookPro display refresh
        // limitation of 45Hz
        if (!test) {
            platforms[i]->continuePatternDisplay();
        }
	}
    frameRateCount++;
    frameRateCount++; // To simulate 60Hz
    processIncoming();
    sendHeartbeat();
}

//--------------------------------------------------------------
void testApp::draw(){
    int j;
    float value = 0;
    for (int i = 0; i < LED_COUNT; i++) {
        for(j = 0; j < platforms.size(); j++){
            ofSetColor(platforms[j]->leds.nonEmbedRed[i],
                       platforms[j]->leds.nonEmbedGreen[i],
                       platforms[j]->leds.nonEmbedBlue[i]);
#ifdef FS_TOWER_EYE
            int boxSize = 25;
            float boxMult = 2.0;
            int offset = 200;
            int rightX = 100;
            int leftX = 500;
            int x, y, increment = boxSize * boxMult;
            if (i < 4) {
                ofDrawEllipse(leftX + i * increment + offset, 30, boxSize / 2, boxSize / 2);
                ofDrawEllipse(rightX - i * increment + offset, 30, boxSize / 2, boxSize / 2);
            } else if (i < 11) {
                ofDrawEllipse(leftX + (10 - i) * increment + offset - increment * 2, (i - 4) / 7 * increment + increment + 30, boxSize / 2, boxSize / 2);
                ofDrawEllipse(rightX - (10 - i) % 7 * increment + offset + increment * 2, (i - 4) / 7 * increment + increment + 30, boxSize / 2, boxSize / 2);
            } else if (i < 18) {
                ofDrawEllipse(leftX + (i - 4) % 7 * increment + offset - increment * 2, (i - 4) / 7 * increment + increment + 30, boxSize / 2, boxSize / 2);
                ofDrawEllipse(rightX - (i - 4) % 7 * increment + offset + increment * 2, (i - 4) / 7 * increment + increment + 30, boxSize / 2, boxSize / 2);
            } else if (i < 81) {
                if ((i - 18) / 9 % 2 == 0) {
                    ofDrawEllipse(leftX + (18 - i) % 9 * increment + offset + increment * 5, (i - 18) / 9 * increment + increment * 3 + 30, boxSize / 2, boxSize / 2);
                    ofDrawEllipse(rightX - (18 - i) % 9 * increment + offset - increment * 5, (i - 18) / 9 * increment + increment * 3 + 30, boxSize / 2, boxSize / 2);
                } else {
                    ofDrawEllipse(leftX + (i - 18) % 9 * increment + offset - increment * 3, (i - 18) / 9 * increment + increment * 3 + 30, boxSize / 2, boxSize / 2);
                    ofDrawEllipse(rightX - (i - 18) % 9 * increment + offset + increment * 3, (i - 18) / 9 * increment + increment * 3 + 30, boxSize / 2, boxSize / 2);
                }
            } else if (i < 88) {
                ofDrawEllipse(leftX + (i - 81) % 7 * increment + offset - increment * 2, (i - 81) / 7 * increment + increment * 10 + 30, boxSize / 2, boxSize / 2);
                ofDrawEllipse(rightX - (i - 81) % 7 * increment + offset + increment * 2, (i - 81) / 7 * increment + increment * 10 + 30, boxSize / 2, boxSize / 2);
            } else if (i < 95) {
                ofDrawEllipse(leftX + (81 - i) % 7 * increment + offset + increment * 4, (i - 81) / 7 * increment + increment * 10 + 30, boxSize / 2, boxSize / 2);
                ofDrawEllipse(rightX - (81 - i) % 7 * increment + offset - increment * 4, (i - 81) / 7 * increment + increment * 10 + 30, boxSize / 2, boxSize / 2);
            } else {
                ofDrawEllipse(leftX + (i - 95) % 5 * increment + offset - increment, (i - 95) / 5 * increment + increment * 12 + 30, boxSize / 2, boxSize / 2);
                ofDrawEllipse(rightX - (i - 95) % 5 * increment + offset + increment, (i - 95) / 5 * increment + increment * 12 + 30, boxSize / 2, boxSize / 2);
            }
#elif defined FS_TOWER || defined FS_HAT || defined FS_TOWN_CENTER
            int boxSize = 13;
            ofDrawRectangle(80 * j + 20, (LED_COUNT - i) * boxSize, boxSize, boxSize);
            ofSetHexColor(0xA0A0A0);
#else
            int boxSize = 40;
            float boxMult = 1.2;
            int x, y, increment = boxSize * boxMult;
            if (i < 10) {
                ofDrawRectangle(i % 2 * increment + 30, i / 2 * increment + increment + 30, boxSize, boxSize);
            } else if (i > 39) {
                ofDrawRectangle((i - 40) % 2 * increment + 7 * increment + 30, (i - 40) / 2 * increment + increment + 30, boxSize, boxSize);
            } else {
                x = 30 + increment * 2;
                y = 30;
                switch (i) {
                    case 10:
                        y += increment * 5;
                        break;
                    case 11:
                        y += increment * 4;
                        break;
                    case 12:
                        y += increment * 3;
                        break;
                    case 13:
                        y += increment * 2;
                        break;
                    case 14:
                        y += increment * 1;
                        break;
                    case 15:
                        break;
                    case 16:
                        x += increment * 1;
                        break;
                    case 17:
                        x += increment * 2;
                        break;
                    case 18:
                        x += increment * 3;
                        break;
                    case 19:
                        x += increment * 4;
                        break;
                    case 20:
                        x += increment * 4;
                        y += increment * 1;
                        break;
                    case 21:
                        x += increment * 3;
                        y += increment * 1;
                        break;
                    case 22:
                        x += increment * 2;
                        y += increment * 1;
                        break;
                    case 23:
                        x += increment * 1;
                        y += increment * 1;
                        break;
                    case 24:
                        x += increment * 1;
                        y += increment * 2;
                        break;
                    case 25:
                        x += increment * 2;
                        y += increment * 2;
                        break;
                    case 26:
                        x += increment * 3;
                        y += increment * 2;
                        break;
                    case 27:
                        x += increment * 4;
                        y += increment * 2;
                        break;
                    case 28:
                        x += increment * 4;
                        y += increment * 3;
                        break;
                    case 29:
                        x += increment * 3;
                        y += increment * 3;
                        break;
                    case 30:
                        x += increment * 2;
                        y += increment * 3;
                        break;
                    case 31:
                        x += increment * 1;
                        y += increment * 3;
                        break;
                    case 32:
                        x += increment * 1;
                        y += increment * 4;
                        break;
                    case 33:
                        x += increment * 1;
                        y += increment * 5;
                        break;
                    case 34:
                        x += increment * 2;
                        y += increment * 5;
                        break;
                    case 35:
                        x += increment * 2;
                        y += increment * 4;
                        break;
                    case 36:
                        x += increment * 3;
                        y += increment * 4;
                        break;
                    case 37:
                        x += increment * 3;
                        y += increment * 5;
                        break;
                    case 38:
                        x += increment * 4;
                        y += increment * 5;
                        break;
                    case 39:
                        x += increment * 4;
                        y += increment * 4;
                        break;
                    default:
                        break;
                }
                ofDrawRectangle(x, y, boxSize, boxSize);
                char arbStr[255];
                sprintf(arbStr, "%i", i);
                ofSetHexColor(0xFFFFFF);
                font.drawString(arbStr, x + boxSize / 2, y + boxSize / 2);
            }
#endif
            font.drawString(patternDisplayName(platforms[j]).c_str(), 80 * j + 5, 680);
        }
    }
    char arbStr[255];
    int textX = 80 * 13;
    if (millis() - heartbeatTimestamp < 400) {
        ofSetHexColor(0xFF0000);
        ofDrawRectangle(textX, 86, 150, 20);
    }
    ofSetHexColor(0xA0A0A0);
    sprintf(arbStr, "Clock: %i", (int)rtc_get());
    font.drawString(arbStr, textX, 70);
    sprintf(arbStr, "Frame rate: %i", (int)ofGetFrameRate() * 2);
    font.drawString(arbStr, textX, 100);
#ifdef FS_TOWER
//    sprintf(arbStr, "X Tilt    : %i Cal  : %i", xTiltSetting, (int)platforms[0]->xTiltCal);
//    font.drawString(arbStr, 80 * j, 130);
//    sprintf(arbStr, "Y Tilt    : %i Cal  : %i", yTiltSetting, (int)platforms[0]->yTiltCal);
//    font.drawString(arbStr, 80 * j, 160);
//    sprintf(arbStr, "Z Tilt    : %i Cal  : %i", zTiltSetting, (int)platforms[0]->zTiltCal);
//    font.drawString(arbStr, 80 * j, 190);
#endif
    int k = 0;
    ofSetHexColor(0x000000);
    sprintf(arbStr, "Message History:");
    font.drawString(arbStr, textX, 220);
    for (list<uint8_t*>::iterator it = messages.begin(); it != messages.end(); it++) {
        uint8_t * data = (uint8_t *)*it;
        stringstream ss;
        ss << patternName(data[0]);
        ss << " s-" << (int)data[1] << " r-" << (int)data[2] << " g-" << (int)data[3] << " b-" << (int)data[4] << " i-" << (int)data[5];
        font.drawString(ss.str(), textX, 240 + 20 * k);
        k++;
	}
    // GPS mapped output
#if ((defined FS_TOWER) || (defined FS_TOWN_CENTER)) && !defined FS_TOWER_EYE
    ofDrawRectangle(850, 400, 320 / 1.5, 480 / 1.5);
#endif
    drawGPSdata();
}

void testApp::drawGPSdata() {
#ifdef FS_TOWER
    float minLatitude = 99999.0, maxLatitude = -99999.0, minLongitude = 99999.0, maxLongitude = -99999.0;
    
	for(int i = 0; i < platforms.size(); i++){
        minLatitude = min (minLatitude, platforms[i]->latitude);
        maxLatitude = max (maxLatitude, platforms[i]->latitude);
        minLongitude = min (minLongitude, platforms[i]->longitude);
        maxLongitude = max (maxLongitude, platforms[i]->longitude);
    }
    float latRange = maxLatitude - minLatitude;
    float lonRange = maxLongitude - minLongitude;
    float aspectRatio = 480.0 / 320.0;
    if (latRange / lonRange > aspectRatio) {
        minLatitude = minLatitude - 0.1 * aspectRatio * latRange;
        maxLatitude = maxLatitude + 0.1 * aspectRatio * latRange;
        minLongitude = minLongitude - 0.1 * 1.0 / aspectRatio * (lonRange + 1.0 / aspectRatio * latRange);
        maxLongitude = maxLongitude + 0.1 * 1.0 / aspectRatio * (lonRange + 1.0 / aspectRatio * latRange);
    } else {
        minLatitude = minLatitude - 0.1 * aspectRatio * (latRange + aspectRatio * lonRange);
        maxLatitude = maxLatitude + 0.1 * aspectRatio * (latRange + aspectRatio * lonRange);
        minLongitude = minLongitude - 0.1 * 1.0 / aspectRatio * lonRange;
        maxLongitude = maxLongitude + 0.1 * 1.0 / aspectRatio * lonRange;
    }
    latRange = maxLatitude - minLatitude;
    lonRange = maxLongitude - minLongitude;
    ofSetHexColor(0xA0A0A0);
	for(int i = 0; i < platforms.size(); i++){
        float x = 850.0 + 320.0 * (float)(platforms[i]->longitude - minLongitude) / lonRange / 1.5;
        float y = 400.0 + 480.0 * (float)(platforms[i]->latitude - minLatitude) / latRange / 1.5;
        ofDrawEllipse(x, y, 20, 20);
    }
#endif
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    
    uint8_t randomFlashData[] = {FS_ID_RANDOM_FLASH, 250, 200, 10, 130, 200};
    uint8_t rainbowChaseData[] = {FS_ID_RAINBOW_CHASE, 120, 130, 100, 130, 240};
    uint8_t organicData[] = {FS_ID_ORGANIC, 10, 0, 0, 0, 0};
    uint8_t descendData[] = {FS_ID_DESCEND, 100, 200, 10, 130, 250};
    uint8_t breatheData[] = {FS_ID_BREATHE, 100, 20, 10, 200, 150};
    uint8_t heartData[] = {FS_ID_HEART, 165, 255, 0, 50, 250};
    uint8_t cylonData[] = {FS_ID_CYLON, 150, 255, 0, 0, 250};
    uint8_t grassWaveData[] = {FS_ID_GRASS_WAVE, 20, 0, 255, 0, 250};
    uint8_t shakeSparkleData[] = {FS_ID_SHAKE_SPARKLE, 100, 200, 0, 220, 220};
    uint8_t ballBounceData[] = {FS_ID_BOUNCING_BALL, 253, 255, 100, 130, 120};
    uint8_t radioTowerData[] = {FS_ID_RADIO_TOWER, 120, 255, 100, 130, 255};
    uint8_t tiltData[] = {FS_ID_TILT, 120, 255, 100, 130, 255};
    uint8_t prismData[] = {FS_ID_PRISM, 120, 255, 100, 130, 255};
    uint8_t spiralData[] = {FS_ID_SPIRAL, 120, 255, 100, 130, 255};
    uint8_t forestData[] = {FS_ID_FOREST_RUN, 120, 255, 0, 0, 255};
    uint8_t animateData[] = {FS_ID_ANIMATE_1, 120, 255, 0, 0, 255};
    uint8_t pongData[] = {FS_ID_PONG, 60, 1, 255, 130, 220};
    uint8_t searchingData[] = {FS_ID_SEARCHING_EYE, 10, 250, 100, 130, 250, 0};
    uint8_t brokenData[] = {FS_ID_BROKEN, 10, 250, 100, 130, 250, 0};
    uint8_t animationData[] = {FS_ID_ANIMATE_1, 10, 0, 0, 0, 0, 0};
    uint8_t bubbleData[] = {FS_ID_BUBBLE_WAVE, 40, 255, 50, 0, 0, 0};
    uint8_t flameData[] = {FS_ID_FLAME, 20, 255, 255, 255, 0, 0};
    uint8_t candleData[] = {FS_ID_CANDLE, 20, 255, 255, 255, 0, 0};
    uint8_t verticalCylonData[] = {FS_ID_CYLON_VERTICAL, 20, 255, 255, 255, 0, 0};
    uint8_t pongCylonData[] = {FS_ID_CYLON_PONG, 20, 255, 255, 255, 0, 0};
    uint8_t matrixData[] = {FS_ID_MATRIX, 20, 0, 255, 0, 255, 0};
    uint8_t spectrumData[] = {FS_ID_SPECTRUM_ANALYZER, 20, 0, 255, 0, 255, 0};
    int i;
    lastDataLength = 6;
    int tiltStep = 3;
    bool done = false;
    bool setting = false;
    switch (key) {
        case 49: // 1
            //memcpy (data, randomFlashData, 6);
            memcpy (data, verticalCylonData, 6);
            break;
        case 50: // 2
            memcpy (data, spectrumData, 6);
            break;
        case 51: // 3
            //memcpy (data, organicData, 6);
            memcpy (data, pongCylonData, 6);
            break;
        case 52: // 4
            //memcpy (data, descendData, 6);
            memcpy (data, matrixData, 6);
            break;
        case 53: // 5
            memcpy (data, heartData, 6);
            break;
        case 54: // 6
            memcpy (data, cylonData, 6);
            break;
        case 55: // 7
            memcpy (data, grassWaveData, 6);
            break;
        case 56: // 8
            memcpy (data, breatheData, 6);
            break;
        case 57: // 9
            memcpy (data, shakeSparkleData, 6);
            break;
        case 48: // 0
            memcpy (data, ballBounceData, 6);
            break;
        case 45: // -
            memcpy (data, radioTowerData, 6);
            break;
        case 61: // =
            memcpy (data, tiltData, 6);
            break;
        case 91: // [
            memcpy (data, prismData, 6);
            break;
        case 93: // ]
            memcpy (data, spiralData, 6);
            break;
        case 92: /* \ */
            memcpy (data, pongData, 6);
            break;
        case 39: // '
            memcpy (data, forestData, 6);
            break;
        case 59: // ;
            memcpy (data, searchingData, 6);
            break;
        case 108: // l
            memcpy (data, brokenData, 6);
            break;
        case 107: // k
            memcpy (data, animationData, 6);
            break;
        case 106: // j
            memcpy (data, bubbleData, 6);
            break;
        case 104: // h
            memcpy (data, flameData, 7);
            break;
        case 109: // m
            memcpy (data, candleData, 7);
            break;
        default:
            done = true;
            break;
    }
    if (done) {
        done = false;
        uint8_t increment = 2;
        switch (key) {
            case 357:
                // Key up
                data[1] = data[1] + increment;
                break;
            case 359:
                // Key down
                data[1] = data[1] - increment;
                break;
            case 356:
                // Key left
                data[5] = data[5] - increment;
                break;
            case 358:
                // Key right
                data[5] = data[5] + increment;
                break;
                /*
                if (key == 358) {
                    // Key right
                    for(i = 0; i < platforms.size(); i++){
                        platforms[i]->triggerPatternChange();
                    }
                } else {
                    // Key left
                    for(i = 0; i < platforms.size(); i++){
                        platforms[i]->advancePatternIntensity(platforms[i]->pattern);
                    }
                }
                done = true;
                break;
                 */
            case 114:
                // R
                data[2] = data[2] + increment;
                break;
            case 103:
                // G
                data[3] = data[3] + increment;
                break;
            case 98:
                // B
                data[4] = data[4] + increment;
                break;
            case 116:
                // T
                data[2] = data[2] - increment;
                break;
            case 104:
                // H
                data[3] = data[3] - increment;
                break;
            case 110:
                // N
                data[4] = data[4] - increment;
                break;
            case 113: // q
                setting = true;
                xTiltSetting += tiltStep;
                break;
            case 97: // a
                setting = true;
                xTiltSetting -= tiltStep;
                break;
            case 119: // w
                setting = true;
                yTiltSetting += tiltStep;
                break;
            case 115: // s
                setting = true;
                yTiltSetting -= tiltStep;
                break;
            case 101: // e
                setting = true;
                zTiltSetting += tiltStep;
                break;
            case 100: // d
                setting = true;
                zTiltSetting -= tiltStep;
                break;
            default:
                break;
        }
    }
    data[6] = 0;
    for(int i = 0; i < platforms.size(); i++){
        if (data[0] == FS_ID_PONG) {
            data[2] = i + 1;
        }
        platforms[i]->setPatternData(data, 7);
#ifdef FS_TOWER
        platforms[i]->animations.isAnimating = data[0] == FS_ID_ANIMATE_1;
        platforms[i]->latitude = 100 * sin (2 * PI * i / 10);
        platforms[i]->longitude = 100 * cos (2 * PI * i / 10);
#endif
        if (data[0] != FS_ID_PONG) {
            //data[6] += 8;
        }
    }
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}

string testApp::patternName(int patternId) {
    stringstream ss;
    switch (patternId) {
        case FS_ID_FULL_COLOR:
            ss << FS_NAME_FULL_COLOR;
            break;
        case FS_ID_SPARKLE:
            ss << FS_NAME_SPARKLE;
            break;
        case FS_ID_DESCEND:
            ss << FS_NAME_DESCEND;
            break;
        case FS_ID_OFF:
            ss << FS_NAME_OFF;
            break;
        case FS_ID_FLASH:
            ss << FS_NAME_FLASH;
            break;
        case FS_ID_FIRE:
            ss << FS_NAME_FIRE;
            break;
        case FS_ID_HEART:
            ss << FS_NAME_HEART;
            break;
        case FS_ID_BREATHE:
            ss << FS_NAME_BREATHE;
            break;
        case FS_ID_ORGANIC:
            ss << FS_NAME_ORGANIC;
            break;
        case FS_ID_CYLON:
            ss << FS_NAME_CYLON;
            break;
        case FS_ID_DROP:
            ss << FS_NAME_DROP;
            break;
        case FS_ID_CHARACTER:
            ss << FS_NAME_CHARACTER;
            break;
        case FS_ID_CYLON_VERTICAL:
            ss << FS_NAME_CYLON_VERTICAL;
            break;
        case FS_ID_CYLON_PONG:
            ss << FS_NAME_CYLON_PONG;
            break;
        case FS_ID_SOUND_ACTIVATE:
            ss << FS_NAME_SOUND_ACTIVATE;
            break;
        case FS_ID_PRISM:
            ss << FS_NAME_PRISM;
            break;
        case FS_ID_PRISM_DISTANCE:
            ss << FS_NAME_PRISM_DISTANCE;
            break;
        case FS_ID_MATRIX:
            ss << FS_NAME_MATRIX;
            break;
        case FS_ID_RAINBOW_CHASE:
            ss << FS_NAME_RAINBOW_CHASE;
            break;
        case FS_ID_RANDOM_FLASH:
            ss << FS_NAME_RANDOM_FLASH;
            break;
        case FS_ID_IMAGE_SCROLL:
            ss << FS_NAME_IMAGE_SCROLL;
            break;
        case FS_ID_STARFIELD:
            ss << FS_NAME_STARFIELD;
            break;
        case FS_ID_SPIRAL:
            ss << FS_NAME_SPIRAL;
            break;
        case FS_ID_TILT:
            ss << FS_NAME_TILT;
            break;
        case FS_ID_SHAKE_SPARKLE:
            ss << FS_NAME_SHAKE_SPARKLE;
            break;
        case FS_ID_SPARKLER:
            ss << FS_NAME_SPARKLER;
            break;
        case FS_ID_GRASS_WAVE:
            ss << FS_NAME_GRASS_WAVE;
            break;
        case FS_ID_RADIO_TOWER:
            ss << FS_NAME_RADIO_TOWER;
            break;
        case FS_ID_BOUNCING_BALL:
            ss << FS_NAME_BOUNCING_BALL;
            break;
        case FS_ID_FOREST_RUN:
            ss << FS_NAME_FOREST_RUN;
            break;
        case FS_ID_PONG:
            ss << FS_NAME_PONG;
            break;
        case FS_ID_BROKEN:
            ss << FS_NAME_BROKEN;
            break;
        case FS_ID_SEARCHING_EYE:
            ss << FS_NAME_SEARCHING_EYE;
            break;
        case FS_ID_BUBBLE_WAVE:
            ss << FS_NAME_BUBBLE_WAVE;
            break;
        case FS_ID_FLAME:
            ss << FS_NAME_FLAME;
            break;
        case FS_ID_CANDLE:
            ss << FS_NAME_CANDLE;
            break;
        default:
            ss << patternId;
            break;
    }
    return ss.str();
}

string testApp::patternDisplayName(furSwarmPatterns* tower) {
    stringstream ss;
    ss << patternName(tower->pattern);
    ss << endl << (int)data[1] << "-" << (int)tower->patternSpeed << endl;
    ss << "red-" << (int)data[2] << endl << "green-" << (int)data[3] << endl << "blue-" << (int)data[4] << endl << "inten-" << (int)data[5];
    return ss.str();
}

//!Process incoming commands
void testApp::processIncoming() {
    xbee.readPacket();
    if (xbee.getResponse().isAvailable()) {
        if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
            processRXResponse();
        } else if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
            xbee.getResponse().getZBTxStatusResponse(txStatus);
            if (txStatus.getDeliveryStatus() == SUCCESS) {
            }
            else {
                for(int i = 0; i < platforms.size(); i++){
                    platforms[i]->failedMessageCount++;
                }
            }
        } else if (xbee.getResponse().isError()) {
            for(int i = 0; i < platforms.size(); i++){
                platforms[i]->failedMessageCount++;
            }
        }
    }
}

//! Process the incoming Coordinator message
void testApp::processRXResponse() {
    uint8_t* data;
    uint8_t dataLength;
    xbee.getResponse().getZBRxResponse(rxResponse);
    // TODO: Check data length for potential errors
    data = rxResponse.getData();
    dataLength = rxResponse.getDataLength();
    if (data[0] == FS_ID_PRISM_DISTANCE && data[1] == 128) {
        heartbeatPeriod = hifreqHeartbeatPeriod;
    } else {
        heartbeatPeriod = defaultHeartbeatPeriod;
    }
    for(int i = 0; i < platforms.size(); i++){
        platforms[i]->initializePattern(data, dataLength);
    }
    uint8_t * dataStore = (uint8_t *)calloc(dataLength, sizeof(uint8_t));
    memcpy(dataStore, data, dataLength);
    messages.push_front(dataStore);
    if (messages.size() > MESSAGE_DISPLAY_LIMIT) {
        messages.pop_back();
    }
}

//! Broadcast a heartbeat if it's been long enough since the last one
void testApp::sendHeartbeat() {
    float frameRate = 0.0;
    uint8_t frameRateByte;
    unsigned long currentTimestamp = millis();
    unsigned long sinceLastHeartbeat = currentTimestamp - heartbeatTimestamp;
    if (sinceLastHeartbeat > heartbeatPeriod) {
        frameRate = (float) frameRateCount / (float) (sinceLastHeartbeat) * 1000.0;
        frameRateByte = (uint8_t) (frameRate + 0.5);
        int i = 0; // Only send from first radio
        memcpy (&heartbeatPayload[frameCountPosition], &frameCount, sizeof(frameCount));
        memcpy (&heartbeatPayload[patternPosition], &platforms[i]->pattern, sizeof(platforms[i]->pattern));
        //memcpy (&heartbeatPayload[voltagePosition], &batteryVoltage, sizeof(batteryVoltage));
        memcpy (&heartbeatPayload[frameRatePosition], &frameRateByte, sizeof(frameRateByte));
        memcpy (&heartbeatPayload[failedMessagePosition], &platforms[i]->failedMessageCount, sizeof(platforms[i]->failedMessageCount));
        xbee.send(heartbeatMessage);
        frameRateCount = 0;
        heartbeatTimestamp = millis();
    }
}

