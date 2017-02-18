// nesnet Particle Photon firmware
// Heavily based on the connectedNES driver by hxlnt

#define NES_CLOCK D1                                // Red wire
#define NES_LATCH D2                                // Orange wire
#define NES_DATA D3                                 // Yellow wire
#define PHOTON_LIGHT 7
#define LATCH_THRESHOLD 20

volatile unsigned char latchedByte;                 // Controller press byte value = one letter in tweet
volatile unsigned char bitCount;                    // A single LDA $4017 (get one bit from "controller press")
volatile unsigned char byteCount;                   // How many bytes have already been printed
volatile unsigned char bytesToTransfer;             // How many bytes are left to print

unsigned char tweetData[192];                       // Array that will hold 192 hex values representing tweet data      
volatile int nesClockCount;
volatile int lastNesClockCount;
volatile unsigned long currentTime;
volatile unsigned long lastTime;
volatile bool isSendingBytes = 0;
volatile bool readyToSendBytes = 0;

volatile byte numLatches;


//////////////////////////////////////////

void setupData() {
    byteCount = 0;                                  // Initialize byteCount at zero, no letters printed to screen
    bytesToTransfer = 0;                            // Initialize bytesToTransfer at zero, no letters waiting to print to screen
    nesClockCount = 1;
    lastNesClockCount = 0;
    numLatches = 0;
    currentTime = micros();
    lastTime = currentTime;
    latchedByte = 0;
    bitCount = 0;
    
    bytesToTransfer = 13;
    tweetData[0] = 0;
    tweetData[1] = 0;
    tweetData[2] = 0; // Prefix with a few zeroes to make sure we're on the same page as our NES friend. (TODO: Probably only need one or maaaybe two.)
    tweetData[3] = 0;
    tweetData[4] = 0;
    tweetData[5] = 'L';
    tweetData[6] = 'E';
    tweetData[7] = 'M';
    tweetData[8] = 'G';
    tweetData[9] = 'U';
    tweetData[10] = 'I';
    tweetData[11] = 'N';
    tweetData[12] = 'S';
   
}

void setup() {
    
    // Don't interpret NES startup/etc as a handshake.. just wait on startup for a lil bit.
    delay(5000);
    

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
}                                        


//////////////////////////////////////////


void myHandler(String event, String data) {
    char inputStr[193];
    data.toCharArray(inputStr, 193);
    tweetData[0] = 0xE8;
    static int i=1;
    for(i=1; i<192; i++) { tweetData[i] = inputStr[i]; }
    memset(&inputStr[0], 0, sizeof(inputStr));
    bytesToTransfer = 192;
    byteCount = 0;
}


////////////////////////////////////////


void ClockNES() {
    if (readyToSendBytes) {
        digitalWrite(NES_DATA, latchedByte & 0x01);
        latchedByte >>= 1;
        bitCount++;
    }
}

    
/////////////////////////////////////////


void LatchNES() {
    
    if (!isSendingBytes) {
        readyToSendBytes = false;
        if (currentTime - lastTime < LATCH_THRESHOLD) {
            numLatches++;
        } else {
            numLatches = 0;
        }
        if (numLatches >= 7) {
            isSendingBytes = true;
            digitalWrite(PHOTON_LIGHT, HIGH);
            setupData();
        }

    } else { 
        readyToSendBytes = true;
        if (byteCount == bytesToTransfer) {
            latchedByte = 0xFF;
            digitalWrite(NES_DATA, latchedByte & 0x01);
            latchedByte >>= 1;
            bitCount = 0;
            isSendingBytes = false;
            readyToSendBytes = false;
            digitalWrite(PHOTON_LIGHT, LOW);
        } else {
            latchedByte = tweetData[byteCount] ^ 0xFF;
            digitalWrite(NES_DATA, latchedByte & 0x01);
            latchedByte >>= 1;
            bitCount = 0;
            byteCount++;
        }
    }
    lastTime = currentTime;
    currentTime = micros();
}
