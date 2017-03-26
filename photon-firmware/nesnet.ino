// This #include statement was automatically added by the Particle IDE.
#include "HttpClient.h"

// nesnet Particle Photon firmware
// Heavily based on the connectedNES driver by hxlnt

#define NES_CLOCK D1                                // Red wire
#define NES_LATCH D2                                // Orange wire
#define NES_DATA D3                                 // Yellow wire
#define PHOTON_LIGHT 7
#define LATCH_THRESHOLD 20
#define FETCH_LATCH_THRESHOLD 40
#define HANDSHAKE_2_WAIT_TIME 0

volatile unsigned char latchedByte = 0;                 // Controller press byte value = one letter in tweet
volatile unsigned char bitCount = 0;                    // A single LDA $4017 (get one bit from "controller press")
volatile unsigned int byteCount = 0;                   // How many bytes have already been printed
volatile unsigned char incomingBitCount = 0;
volatile unsigned int incomingByteCount = 0;

volatile unsigned char tweetData[550];                       // Array that will hold 192 hex values representing tweet data      
char receivedBytes[100];
volatile unsigned long currentTime = 0;
volatile unsigned long lastTime = 0;
volatile bool readyToSendBytes = 0;
volatile bool hasGottenHandshake = 0;
volatile bool hasGottenHandshake1 = 0;
volatile bool receivingData = 0;
volatile bool finishedReceivingData = 0;
volatile bool currentBit = 0;
volatile unsigned char currentByte = 0;
volatile bool gazornenplat = 0;
volatile unsigned long lastLoadBearingLatch = 0;
volatile unsigned long experiment;
volatile unsigned long a, b;
volatile bool dongs = 0;
volatile bool hasByteLatched = 0;
volatile unsigned char repeatCount = 0;
HttpClient http;

volatile byte numLatches = 0;

// Headers currently need to be set at init, useful for API keys etc.
http_header_t headers[] = {
    { "Accept" , "*/*"},
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};

http_request_t request;
http_response_t response;

//////////////////////////////////////////

void setupData() {
    byteCount = 0;                                  // Initialize byteCount at zero, no letters printed to screen
    numLatches = 0;
    currentTime = 9000;
    lastTime = currentTime-9000;
    latchedByte = 0;
    bitCount = 0;
    finishedReceivingData = 0;
    incomingBitCount = 0;
    incomingByteCount = 0;
    currentByte = 0;
    lastLoadBearingLatch = 0;
    currentBit = 0;
    receivingData = 0;
    hasByteLatched = 0;

    dongs = 0;
    
    static int i;
    for (i = 0; i < 100; i++)
        receivedBytes[i] = 0;
    for (i = 0; i < 550; i++)
        tweetData[i] = 0;
        
    experiment = 0;
    repeatCount = 0;
        
        
   
}

void setup() {
    
    // Don't interpret NES startup/etc as a handshake.. just wait on startup for a lil bit.
    delay(2000);
    Serial.begin(9600);
    

    pinMode(NES_CLOCK, INPUT);                      // Set NES controller red wire (clock) as an input
    pinMode(NES_LATCH, INPUT);                      // Set NES controller orange wire (latch) as an input
    pinMode(NES_DATA, OUTPUT);                      // Set NES controller yellow wire (data) as an output
    
    attachInterrupt(NES_CLOCK, ClockNES, FALLING);  // When NES clock ends, execute ClockNES
    attachInterrupt(NES_LATCH, LatchNES, RISING);   // When NES latch fires, execure LatchNES
    
    pinMode(PHOTON_LIGHT, OUTPUT);                             // Turn off the Photon's on-board LED
    digitalWrite(PHOTON_LIGHT, LOW);                           //
    
    //Particle.variable("received", receivedBytes, STRING);
    //Particle.variable("response", &tweetData[5], STRING);
    
    setupData();
}


//////////////////////////////////////////


void loop() {                                       // 'Round and 'round we go    
    if (finishedReceivingData == true && gazornenplat == false) {
        // char buffer[256];
        //sprintf(buffer, "NES Debug data received: %u, %u, %u, %u in %lu: %s", receivedBytes[0], receivedBytes[1], receivedBytes[2], receivedBytes[3], b-a);
        //Particle.publish("dataReceived", buffer);
        gazornenplat = true;
        

    }
    if (finishedReceivingData && !dongs) {
        if (strcmp(receivedBytes, "/test") == 0) {
            tweetData[0] = 255;
            tweetData[1] = 'W';
            tweetData[2] = 'F';
            tweetData[3] = 200;
            tweetData[4] = 1;
            for (int i = 0; i < 7; i++) {
                tweetData[5+i] = "TEST OK"[i];
            }
            dongs = true;
        } else {
            GetNetResponse();
            // Skip the first null byte 
            response.body.getBytes((unsigned char*)&tweetData[9], min(response.body.length()+1, 512));
            tweetData[4] = 255; // Add a garbage byte before to be ignored.
            // Two more garbage bytes to ignore - past this point, the NES seems to figure out what's going on.
            // TODO: Really need to understand why I need to do this.
            tweetData[5] = 'W';
            tweetData[6] = 'F';
            // Response code
            tweetData[7] = response.status & 0xff;
            tweetData[8] = (unsigned char)((response.status>>8)+1) & 0xff; // HACK: Add 1 to the high byte so that it is never 0. (= null; confuses us + the driver)
            dongs = true;
        }


    }
}                                        

////////////////////////////////////////


void ClockNES() {
    if (readyToSendBytes && hasByteLatched) {
        digitalWrite(NES_DATA, latchedByte & 0x01);
        latchedByte >>= 1;
        bitCount++;
    }
}

/////////////////////////////////////////

void GetNetResponse() {
    String fullUrl = String(receivedBytes);
    int colonPos = fullUrl.indexOf(':');
    int slashPos = fullUrl.indexOf('/');
    if (colonPos != -1) {
        request.hostname = fullUrl.substring(0, colonPos);
        request.port = fullUrl.substring(colonPos+1, slashPos).toInt();
        request.path = fullUrl.substring(slashPos);
    } else {
        request.port = 80;
        request.hostname = fullUrl.substring(0, slashPos);
        request.path = fullUrl.substring(slashPos);
    }

    // The library also supports sending a body with your request:
    //request.body = "{\"key\":\"value\"}";

    // Get request
    http.get(request, response, headers);
}
    
/////////////////////////////////////////


void LatchNES() {
    
    if (!hasGottenHandshake) {
        readyToSendBytes = false;
        if (currentTime - lastTime < LATCH_THRESHOLD) {
            numLatches++;
        } else {
            numLatches = 0;
        }
        if (numLatches >= 7) {
            hasGottenHandshake = true;
            gazornenplat = false;
            
            // blah
            receivingData = true;
            currentBit = 0;
            currentByte = 0;
            //
            
            digitalWrite(PHOTON_LIGHT, HIGH);
            setupData();
        }
    } else if (hasGottenHandshake && !receivingData) {
        receivingData = true; // Only purpose of this is to wait until the next latch signal to start doing something.
        currentByte = 0;
        currentBit = 1; // TODO: Why is this necessary?
        a = micros();
    } else if (receivingData && !finishedReceivingData) {
        if (micros() - lastLoadBearingLatch < FETCH_LATCH_THRESHOLD) {
            // If we get a second latch signal before our next expected one, this signifies a one.
            // If we don't, currentBit will keep stay at zero, and go again.
            currentBit = 1;
        } else {
            currentByte += (currentBit << incomingBitCount);
            incomingBitCount++;
            if (incomingBitCount == 8) {
                incomingBitCount = 0;
                receivedBytes[incomingByteCount] = currentByte;
                incomingByteCount++;
                if (currentByte == 0 && incomingByteCount > 1) { // TODO: Is this ok?
                    finishedReceivingData = true;
                    b = micros();
                }
                currentByte = 0;
                
            }
            currentBit = 0;
            lastLoadBearingLatch = micros();
        }

    } else if (dongs) { 
        readyToSendBytes = true;
        if (byteCount > 7 && tweetData[byteCount] == '\0') {
            latchedByte = 0xff;
            digitalWrite(NES_DATA, latchedByte & 0x01);
            latchedByte >>= 1;
            bitCount = 0;
            hasGottenHandshake = false;
            readyToSendBytes = false;
            digitalWrite(PHOTON_LIGHT, LOW);
        } else {
            latchedByte = tweetData[byteCount] ^ 0xFF;
            digitalWrite(NES_DATA, latchedByte & 0x01);
            latchedByte >>= 1;
            repeatCount++;
            bitCount = 0u;
            if (repeatCount >= 3u) {
                repeatCount = 0u;
                byteCount++;
            }
        }
        hasByteLatched = true;
    }
    lastTime = currentTime;
    currentTime = micros();
    
}
