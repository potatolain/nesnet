#include "lib/neslib.h"
#include "../../src/nesnet.h"

#define POSITION_URL "http://192.168.1.201:3000/update"
#define SPRITE_INTERNET 0x20
#define SPRITE_PLAYER 0x10

#define DUMMY_SONG 0
#define SFX_BOING 0 

#define REQUEST_DELAY 50

// Globals! Defined as externs in src/globals.h
unsigned char currentPadState;
unsigned char i;
char currentMessage[16];
char dataBuffer[4];

// Local to this file.
static unsigned char showMessageA, playerX, playerY, waitCycle;
static unsigned char playMusic;
static unsigned char chrBank;
static unsigned char statusCode;
static char screenBuffer[20];

// Put a string on the screen at X/Y coordinates given in adr.
void put_str(unsigned int adr, const char *str) {
	vram_adr(adr);
	while(1) {
		if(!*str) break;
		vram_put((*str++)-0x20);//-0x20 because ASCII code 0x20 is placed in tile 0 of the CHR
	}
}

void write_screen_buffer(unsigned char x, unsigned char y, char* data) {
	screenBuffer[0] = MSB(NTADR_A(x, y)) | NT_UPD_HORZ;
	screenBuffer[1] = LSB(NTADR_A(x, y));
	screenBuffer[2] = 16u;
	for (i = 0; data[i] != '\0'; ++i) 
		screenBuffer[i+3u] = data[i]-0x20;
	screenBuffer[19] = NT_UPD_EOF;
	set_vram_update(screenBuffer);
}

// Main entry point for the application.
void main(void) {

	showMessageA = 0;
	playMusic = 1;

	// Queue up our dummy song and start playing it.
	music_play(DUMMY_SONG);
	music_pause(playMusic);

	pal_col(1,0x19);//set dark green color
	pal_col(17,0x19);


	// Show a message to the user.
	put_str(NTADR_A(2,8), "Hello world!");

	put_str(NTADR_A(2, 2), "Waiting for NESNet...");
	ppu_on_all();
	while (!nesnet_check_connected()) {
		ppu_wait_nmi();
	}
	ppu_off();
	// whitespace to line up with above, to cover the ... at the end.
	put_str(NTADR_A(2, 2), "Nesnet Connected!    ");
	ppu_on_all();

	playerX = 50;
	playerY = 50;

	waitCycle = 200;


	// Now we wait for input from the user, and do dumb things!
	while(1) {

		currentPadState = nesnet_pad_poll();
		if (currentPadState & PAD_UP) {
			playerY -= 5;
		} else if (currentPadState & PAD_DOWN) {
			playerY += 5;
		}

		if (currentPadState & PAD_LEFT) {
			playerX -= 5;
		} else if (currentPadState & PAD_RIGHT) {
			playerX += 5;
		}
		oam_spr(playerX, playerY, 0x03, 0, SPRITE_PLAYER);

		// Constantly run http gets to get latest position... waitCycle inserts a small delay
		if (http_request_complete()) { 
			if (waitCycle == REQUEST_DELAY && http_response_code() == 200) {
				oam_spr(currentMessage[0], currentMessage[1], 0x20, 0, SPRITE_INTERNET);
			}

			if (waitCycle == 0) {
				dataBuffer[0] = playerX;
				dataBuffer[1] = playerY;
				// TODO: Direction
				http_post(POSITION_URL, dataBuffer, 4, currentMessage, 8);
				waitCycle = REQUEST_DELAY;
			} else {
				waitCycle--;
			}
		}
		nesnet_do_cycle();
		ppu_wait_nmi();


	}
}