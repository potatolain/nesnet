// nesnet Particle Photon firmware
// Heavily based on the connectedNES driver by hxlnt

// NOTE: HttpClient is released under LGPL. See the full license in lib/http_client. 
#include "lib/http_client/HttpClient.h"

#define NES_CLOCK D1                                // Red wire
#define NES_LATCH D2                                // Orange wire
#define NES_DATA D3                                 // Yellow wire
#define PHOTON_LIGHT 7
#define LATCH_THRESHOLD 20
#define FETCH_LATCH_THRESHOLD 80
#define HANDSHAKE_2_WAIT_TIME 0

// Available request types 
#define HTTP_GET 		'G'
#define HTTP_PUT 		'E'
#define HTTP_POST		'A'
#define HTTP_DELETE		'C'


// NOTE: Everything being volatile like this is really sloppy. It's probably costing some time. (Though, maybe not much since
// the photon can run laps around the NES...)
volatile unsigned char latchedByte = 0;                 // Controller press byte value = one letter in tweet
volatile unsigned char bitCount = 0;                    // A single LDA $4017 (get one bit from "controller press")
volatile unsigned int byteCount = 0;                   // How many bytes have already been printed
volatile unsigned char incomingBitCount = 0;
volatile unsigned int incomingByteCount = 0;
volatile unsigned int incomingPostByteCount = 0;

volatile unsigned char tweetData[550];                       // Array that will hold 192 hex values representing tweet data      
char receivedBytes[500];
char receivedPostData[1000];
volatile char currentRequestType;
volatile unsigned long currentTime = 0;
volatile unsigned long lastTime = 0;
volatile bool readyToSendBytes = 0;
volatile bool hasGottenHandshake = 0;
volatile bool hasGottenHandshake1 = 0;
volatile bool receivingData = 0;
volatile bool finishedReceivingData = 0;
volatile bool finishedReceivingPostData = 0;
volatile int postDataLength = 0;
volatile bool currentBit = 0;
volatile unsigned char currentByte = 0;
volatile bool gazornenplat = 0;
volatile unsigned long lastLoadBearingLatch = 0;
volatile unsigned long experiment;
volatile unsigned long a, b;
volatile bool hasFormattedData = 0;
volatile bool hasByteLatched = 0;
volatile unsigned char repeatCount = 0;
volatile bool gotPostDataLength = 0;
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
    finishedReceivingPostData = 0;
    incomingBitCount = 0;
    incomingByteCount = 0;
    currentByte = 0;
    lastLoadBearingLatch = 0;
    currentBit = 0;
    receivingData = 0;
    hasByteLatched = 0;
    postDataLength = 0;
    incomingPostByteCount = 0;
    gotPostDataLength = 0;
    currentRequestType = HTTP_GET;

    hasFormattedData = 0;
    
    static int i;
    for (i = 0; i < 100; i++)
        receivedBytes[i] = 0;
    for (i = 0; i < 550; i++)
        tweetData[i] = 0;
    for (i = 0; i < 1000; i++)
        receivedPostData[i] = 0;
        
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
    
    setupData();
}


//////////////////////////////////////////


void loop() {                                       // 'Round and 'round we go    
    if (finishedReceivingData == true && gazornenplat == false) {
        char buffer[256];
        sprintf(buffer, "NES Debug data received: %u, %u, %u, %u in %lu: %s", receivedBytes[0], receivedBytes[1], receivedBytes[2], receivedBytes[3], b-a, receivedBytes);
        Particle.publish("dataReceived", buffer);
        gazornenplat = true;
        

    }
    if (finishedReceivingData && (finishedReceivingPostData || currentRequestType == HTTP_GET || currentRequestType == HTTP_DELETE) && !hasFormattedData) {
        int16_t temp;
        if (strcmp(receivedBytes, "G/test") == 0) {
            tweetData[0] = 255;
            tweetData[1] = 'W';
            tweetData[2] = 'F';
            tweetData[3] = 200;
            tweetData[4] = 1; // NOTE: The 1s here are actually zeroes... we have a hack in place that decrements this. See comment in main http section.
            tweetData[5] = 7;
            tweetData[6] = 1;
            for (int i = 0; i < 7; i++) {
                tweetData[7+i] = "TEST OK"[i];
            }
            hasFormattedData = true;
        } else {
            GetNetResponse();

            // Response body length is actually a 32 bit int... if we overflow what fits in a 16 bit int (there are probably other problems, but) try to do the right thing.
            temp = response.body.length();
            if (temp != response.body.length()) {
                temp = 32767;
            }

            // Skip the first null byte 
            response.body.getBytes((unsigned char*)&tweetData[11], min(response.body.length()+1, 512));
            tweetData[4] = 255; // Add a garbage byte before to be ignored.
            // Two more garbage bytes to ignore - past this point, the NES seems to figure out what's going on.
            // TODO: Really need to understand why I need to do this.
            tweetData[5] = 'W';
            tweetData[6] = 'F';
            // Response code
            tweetData[7] = response.status & 0xff;
            tweetData[8] = (unsigned char)((response.status>>8)+1) & 0xff; // HACK: Add 1 to the high byte so that it is never 0. (= null; confuses us + the driver)
            tweetData[9] = temp & 0xff;
            tweetData[10] = (unsigned char)((temp>>8)+1) & 0xff; // TODO: Remove this hack and the one above. It shouldn't be necessary anymore.
            hasFormattedData = true;
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
    // Url skips the first character
    String fullUrl = String(receivedBytes+1);
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

    // What kind of request do we wanna make today? Or... do we need to get more from the NES?
    if (receivedBytes[0] == HTTP_GET) {
        // Get request
        request.body = NULL;
        http.get(request, response, headers);
        // TODO: Implement PUT/POST... Thinking wait for /0 for the main string, then 1 (or 2?) byte length, then data.
    } else if (receivedBytes[0] == HTTP_POST) {
        request.body = String(receivedPostData);
        http.post(request, response, headers);
    } else if (receivedBytes[0] == HTTP_PUT) {
        request.body = String(receivedPostData);
        http.put(request, response, headers);
    } else if (receivedBytes[0] == HTTP_DELETE) { 
        request.body = NULL;
        http.del(request, response, headers);
    } else {
        Particle.publish("unkEvtType", receivedBytes);
    }
}
    
/////////////////////////////////////////


void LatchNES() {
    
    if (!hasGottenHandshake) {
        readyToSendBytes = false;
        if (currentTime - lastTime < LATCH_THRESHOLD) {
            numLatches++;
        } else {
            numLatches = 1;
        }
        if (numLatches >= 8) {
            hasGottenHandshake = true;
            gazornenplat = false;
            
            receivingData = true;
            currentBit = 0;
            currentByte = 0;
            
            digitalWrite(PHOTON_LIGHT, HIGH);
            setupData();
        }
    } else if (hasGottenHandshake && !receivingData) {
        receivingData = true; // Only purpose of this is to wait until the next latch signal to start doing something.
        currentByte = 0;
        currentBit = 1; // TODO: Why is this necessary?
        a = micros();
    } else if (receivingData && (!finishedReceivingData || ((currentRequestType == HTTP_POST || currentRequestType == HTTP_PUT) && !finishedReceivingPostData))) {
        if (micros() - lastLoadBearingLatch < FETCH_LATCH_THRESHOLD) {
            // If we get a second latch signal before our next expected one, this signifies a one.
            // If we don't, currentBit will keep stay at zero, and go again.
            currentBit = 1;
        } else {
            currentByte += (currentBit << incomingBitCount);
            incomingBitCount++;
            if (incomingBitCount == 8) {
                incomingBitCount = 0;
                if (incomingByteCount == 0)
                    currentRequestType = currentByte;
                if (!finishedReceivingData) {
                    receivedBytes[incomingByteCount] = currentByte;
                    incomingByteCount++;
                } else { 
                    if (incomingPostByteCount == 0) {
                        postDataLength = currentByte & 0xff;
                    } else if (incomingPostByteCount == 1) {
                        postDataLength += (currentByte << 8);
                        gotPostDataLength = 1;
                    } else {
                        receivedPostData[incomingPostByteCount-2] = currentByte;
                    }
                    incomingPostByteCount++;
                }
                if (currentByte == 0 && incomingByteCount > 1 && !finishedReceivingData) { // TODO: Do length-based transfer, instead of null-terminated strings. Imagine binary data for payload.
                    finishedReceivingData = true;
                    b = micros();
                }
                if (gotPostDataLength && incomingPostByteCount-2 == postDataLength && !finishedReceivingPostData) {
                    finishedReceivingPostData = true;

                }
                currentByte = 0;
                
            }
            currentBit = 0;
            lastLoadBearingLatch = micros();
        }
    } else if (hasFormattedData) { 
        readyToSendBytes = true;
        if (byteCount == response.body.length() + 11) {
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
