// This is a test program for some NES networking stuff I'm gonna do.
#include "lib/neslib.h"
#include "../src/nesnet.h"

#define GAME_STATE_INIT 0
#define GAME_STATE_HOME 1
#define GAME_STATE_FORUM 2
#define GAME_STATE_TOPIC 3

#define ARROW_CHR_ID 255
#define FIRST_CUSTOM_URL_CHAR 31


static unsigned char theMessage[512];
static unsigned char currentUrl[100];
static unsigned char screenBuffer[20]; // Same data, but transformed for the screen, and with 3 bytes of prefix for neslib.
static unsigned char currentPadState;
static unsigned char callCount, i;
static unsigned char *currentChar;
static unsigned int offset;
static unsigned char forumIds[100]; // Support up to 50 forums with 0-99 ids.
static unsigned char forumIdA;
static unsigned char forumIdB;
static unsigned char currentForumId = 0;
static unsigned char gameState = GAME_STATE_INIT;
static unsigned char currentForumPosition = 0;
static unsigned char currentTopicPosition = 0;
static unsigned char totalForumCount = 0;
static unsigned char totalTopicCount = 0;
static unsigned char hasHitColon = FALSE;
static int resCode;

// Forward declarations of some functions that control game flow. For readability, they are defined later in the file. 
// (Purist note: these probably belong in a header file. I may do that at some point, if I stop being lazy.)
void doInit();
void showHome();
void doHome();
void showForum();
void doForum();
void showTopic();
void doTopic();

void put_str(unsigned int adr,const char *str)
{
	vram_adr(adr);

	while(1)
	{
		if(!*str) break;

		vram_put((*str++)-0x20);//-0x20 because ASCII code 0x20 is placed in tile 0 of the CHR
	}
}

void set_current_url() {
	memcpy(currentUrl, "cpprograms.net/devnull/nesdev/", FIRST_CUSTOM_URL_CHAR-1);
	currentUrl[FIRST_CUSTOM_URL_CHAR] = '\0';
}

void write_screen_buffer(unsigned char x, unsigned char y, char* data) {
	screenBuffer[0] = MSB(NTADR_A(x, y)) | NT_UPD_HORZ;
	screenBuffer[1] = LSB(NTADR_A(x, y));
	screenBuffer[2] = 16u;
	for (i = 0; data[i] != '\0'; i++) 
		screenBuffer[i+3u] = data[i]-0x20;
	screenBuffer[19] = NT_UPD_EOF;
	set_vram_update(screenBuffer);
}

void clear_screen() {
	vram_adr(0x2060);
	vram_fill(0, 0x03a0);
}

// Junky quick-n-dirty convert integer to string, to show an error code on our error screen.
// Hat tip: http://stackoverflow.com/questions/9655202/how-to-convert-integer-to-string-in-c
char* itoa(int i, char b[]){
    char const digit[] = "0123456789";
    char* p = b;
	int shifter;
    if(i<0){
        *p++ = '-';
        i *= -1;
    }
    shifter = i;
    do{ //Move to where representation ends
        ++p;
        shifter = shifter/10;
    }while(shifter);
    *p = '\0';
    do{ //Move back, inserting digits as u go
        *--p = digit[i%10];
        i = i/10;
    }while(i);
    return b;
}

void main(void) {
	//rendering is disabled at the startup, the palette is all black

	pal_col(1,0x19);//set dark green color (TODO: There's a more sane way to do setup)
	pal_col(17, 0x19);


	put_str(NTADR_A(4,2),"NESDev Forum Browser!");
	put_str(NTADR_A(7,4), "By cppchriscpp");
	put_str(NTADR_A(3,27), "Inspired by ConnectedNES");
	put_str(NTADR_A(2,9),"Press start to browse the");
	put_str(NTADR_A(2,10),"most recent forum posts.");


	ppu_on_all();//enable rendering
	callCount = i = 0;
	ppu_wait_frame();

	while(1) {
		switch (gameState) {
			case GAME_STATE_INIT:
				doInit();
				break;
			case GAME_STATE_HOME:
				doHome();
				break;
			case GAME_STATE_FORUM:
				doForum();
				break;
			case GAME_STATE_TOPIC:
				doTopic();
				break;
		} 
	}
}

void doInit() {
	ppu_wait_frame();
	currentPadState = pad_trigger(0);
	if (currentPadState & PAD_START) {
		showHome();
	} else {
		set_vram_update(NULL);
	}

}

void hide_pointer() {
	oam_spr(0, 0xff, 0, 0, 0);
}

void showHome() {
	hide_pointer();
	ppu_off();
	clear_screen();
	ppu_on_all();

	write_screen_buffer(2, 12, "Loading...");

	ppu_wait_frame(); // Flush output to make sure we see this on screen first.
	set_vram_update(NULL);

	http_get("cpprograms.net/devnull/nesdev/", theMessage);


	ppu_off();
	clear_screen();
	offset = 0x2081;
	vram_adr(offset);
	currentChar = &theMessage[0];
	forumIdA = 0;
	forumIdB = 0;
	currentForumId = 0;

	while (*currentChar != '\0') {
		if (forumIdA == 0) {
			forumIdA = *currentChar;
		} else if (forumIdB == 0) {
			forumIdB = *currentChar;
			if (*currentChar == ':') {
				forumIds[currentForumId<<1 + 1] = ' ';
			} else {
				forumIds[currentForumId<<1 + 1] = *currentChar;
				currentChar++; // Force-skip the colon after this char.

			}
			forumIds[currentForumId<<1] = forumIdA;
			totalForumCount++;
			currentForumId++;
		} else {
			vram_put(*currentChar-0x20);
		}
		currentChar++;
		if (*currentChar == '|') {
			offset += 0x20;
			vram_adr(offset);
			currentChar++;
			forumIdA = forumIdB = 0;
		}
	}

	ppu_on_all();
	gameState = GAME_STATE_HOME;
}

void doHome() {
	ppu_wait_frame();
	currentPadState = pad_trigger(0);
	// Undocumented feature - use select to restart.
	if (currentPadState & PAD_SELECT) {
		showHome();
	} else {
		set_vram_update(NULL);
	}

	if (currentPadState & PAD_UP && currentForumPosition > 0) {
		currentForumPosition--;
	} else if (currentPadState & PAD_DOWN && currentForumPosition < totalForumCount-1) {
		currentForumPosition++;
	}

	if (currentPadState & PAD_A) {
		showForum();
	}

	oam_spr(0, 32 + (currentForumPosition<<3), ARROW_CHR_ID, 0, 0);
}

void showForum() {
	hide_pointer();
	ppu_off();
	clear_screen();
	ppu_on_all();
	
	write_screen_buffer(2, 12, "Loading...");

	ppu_wait_frame(); // Flush output to make sure we see this on screen first.
	set_vram_update(NULL);

	set_current_url();
	currentUrl[FIRST_CUSTOM_URL_CHAR-1] = '/'; // TODO: Fix issues with \0 eating characters and making this into a not-url.
	currentUrl[FIRST_CUSTOM_URL_CHAR] 	= '?';
	currentUrl[FIRST_CUSTOM_URL_CHAR+1] = 'f';
	currentUrl[FIRST_CUSTOM_URL_CHAR+2] = '=';
	currentUrl[FIRST_CUSTOM_URL_CHAR+3] = forumIds[currentForumPosition<<1];
	currentUrl[FIRST_CUSTOM_URL_CHAR+4] = forumIds[currentForumPosition<<1 + 1];
	currentUrl[FIRST_CUSTOM_URL_CHAR+5] = '\0';

	resCode = http_get(currentUrl, theMessage);


	ppu_off();
	clear_screen();
	offset = 0x2081;
	vram_adr(offset);
	currentChar = &theMessage[0];
	forumIdA = 0;
	forumIdB = 0;
	currentForumId = 0;

	while (*currentChar != '\0') {
		
		// For now, just skip the id. We'll get there.
		if (*currentChar == ':') {
			hasHitColon = TRUE;
		} else if (hasHitColon) {
			vram_put(*currentChar-0x20);
		}
		currentChar++;
		if (*currentChar == '|') {
			hasHitColon = FALSE;
			offset += 0x20;
			vram_adr(offset);
			currentChar++;
			totalTopicCount++;
		}
	}

	ppu_on_all();
	gameState = GAME_STATE_FORUM;
}


void doForum() {
	ppu_wait_frame();
	currentPadState = pad_trigger(0);
	// Undocumented feature - use select to restart.
	if (currentPadState & PAD_SELECT) {
		showForum();
	} else {
		set_vram_update(NULL);
	}

	if (currentPadState & PAD_UP && currentTopicPosition > 0) {
		currentTopicPosition--;
	} else if (currentPadState & PAD_DOWN && currentTopicPosition < totalTopicCount-1) {
		currentTopicPosition++;
	}

	if (currentPadState & PAD_A) {
		showTopic();
	}

	// B to go back.
	if (currentPadState & PAD_B) {
		showHome();
	}

	oam_spr(0, 32 + (currentTopicPosition<<3), ARROW_CHR_ID, 0, 0);

}

void showTopic() {

}

void doTopic() {

}