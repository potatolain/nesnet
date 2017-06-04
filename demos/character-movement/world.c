#include "lib/neslib.h"
#include "../../src/nesnet.h"

// NOTE: You have to change the URL here to the URL of your PC. Check the output from the nodejs server on startup - it should
// show the server address. Replace this with the address you want.
#define POSITION_URL "http://192.168.1.201:3000/update"
#define SPRITE_INTERNET 0x00
#define SPRITE_PLAYER 0x10
#define TILE_INTERNET 0xe0
#define TILE_PLAYER 0xe4
#define MOVEMENT_SPEED 2

#define DUMMY_SONG 0
#define SFX_BOING 0 

#define REQUEST_DELAY 50

// Globals!
unsigned char currentPadState;
unsigned char i;
unsigned char frameCounter;
char currentMessage[16];

// Defined in crt0.asm with a binary file
extern const char main_palette[16];

// Local to this file.
static unsigned char showMessageA, playerX, playerY, waitCycle;
static unsigned char playMusic;
static unsigned char chrBank;
static unsigned char animOffset;
static unsigned char statusCode;
static char screenBuffer[20];
static char dataBuffer[4];

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

	pal_bg(main_palette);
	pal_spr(main_palette);


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
	animOffset = 0;
	frameCounter = 0;


	// Now we wait for input from the user, and do dumb things!
	while(1) {

		currentPadState = nesnet_pad_poll();
		if (currentPadState & PAD_UP) {
			playerY -= MOVEMENT_SPEED;
		} else if (currentPadState & PAD_DOWN) {
			playerY += MOVEMENT_SPEED;
		}

		if (currentPadState & PAD_LEFT) {
			playerX -= MOVEMENT_SPEED;
		} else if (currentPadState & PAD_RIGHT) {
			playerX += MOVEMENT_SPEED;
		}

		// Alternate between two animations for each sprite.
		if (frameCounter & 0x10) {
			animOffset = 0;
		} else {
			animOffset = 2;
		}
		
		oam_spr(playerX, playerY, TILE_PLAYER+animOffset, 3, SPRITE_PLAYER);
		oam_spr(playerX+8, playerY, TILE_PLAYER+animOffset+1, 3, SPRITE_PLAYER+4);
		oam_spr(playerX, playerY+8, TILE_PLAYER+animOffset+16, 3, SPRITE_PLAYER+8);
		oam_spr(playerX+8, playerY+8, TILE_PLAYER+animOffset+17, 3, SPRITE_PLAYER+12);

		// Hackily animate the crab every frame, assuming he's on screen.
		(*(char*)(0x201 + SPRITE_INTERNET)) = TILE_INTERNET+animOffset;
		(*(char*)(0x205 + SPRITE_INTERNET)) = TILE_INTERNET+animOffset+1;
		(*(char*)(0x209 + SPRITE_INTERNET)) = TILE_INTERNET+animOffset+16;
		(*(char*)(0x20d + SPRITE_INTERNET)) = TILE_INTERNET+animOffset+17;

		// Constantly run http gets to get latest position... waitCycle inserts a small delay
		if (http_request_complete()) { 
			if (waitCycle == REQUEST_DELAY && http_response_code() == 200) {
				oam_spr(currentMessage[0], currentMessage[1], TILE_INTERNET+animOffset, 3, SPRITE_INTERNET);
				oam_spr(currentMessage[0]+8, currentMessage[1], TILE_INTERNET+animOffset+1, 3, SPRITE_INTERNET+4);
				oam_spr(currentMessage[0], currentMessage[1]+8, TILE_INTERNET+animOffset+16, 3, SPRITE_INTERNET+8);
				oam_spr(currentMessage[0]+8, currentMessage[1]+8, TILE_INTERNET+animOffset+17, 3, SPRITE_INTERNET+12);

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
		frameCounter++;


	}
}