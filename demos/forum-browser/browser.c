// Test program to demonstrate NESNet
// Forum browser for the nesdev.com forums using RSS feeds as a source.
#include "lib/neslib.h"
#include "../../src/nesnet.h"

#define GAME_STATE_INIT 0
#define GAME_STATE_HOME 1
#define GAME_STATE_FORUM 2
#define GAME_STATE_TOPIC 3
#define GAME_STATE_ERROR 100

#define ARROW_CHR_ID 255
#define FIRST_CUSTOM_URL_CHAR 38


static unsigned char theMessage[512];
static unsigned char currentUrl[100];
static char screenBuffer[20]; // Same data, but transformed for the screen, and with 3 bytes of prefix for neslib.
static unsigned int topicIds[20]; // Support up to 20 topics with int ids.
#pragma bssseg (push,"ZEROPAGE")
#pragma dataseg(push,"ZEROPAGE")

static unsigned char currentPadState, _currentPadState, lastPadState, tempPadState;
static unsigned char callCount, i, j;
static unsigned int ii, jj;
static unsigned char *currentChar;
static unsigned int offset;
static unsigned char forumIds[100]; // Support up to 50 forums with 0-99 ids.
static unsigned char forumIdA;
static unsigned char forumIdB;
static unsigned char currentForumId = 0;
static unsigned char gameState = GAME_STATE_INIT;
static unsigned char lastGameState = GAME_STATE_HOME;
static unsigned char currentForumPosition = 0;
static unsigned char currentTopicPosition = 0;
static unsigned char totalForumCount = 0;
static unsigned char totalTopicCount = 0;
static unsigned char currentTopicPost = 0;
static unsigned char numberOfPosts = 0;
static unsigned char musicIsPaused = 1;
static unsigned char nesnetConnected = 0, nesnetConnectionAttempts = 0;
static unsigned char hasHitColon = FALSE;
static int resCode;
static unsigned char hasGrabbedCount = 0, hasShownAuthor = 0, hasShownDate = 0;

#pragma bssseg (push,"BSS")
#pragma dataseg(push,"BSS")

// Forward declarations of some functions that control game flow. For readability, they are defined later in the file. 
// (Purist note: these probably belong in a header file. I may do that at some point, if I stop being lazy.)
void do_init();
void show_home();
void do_home();
void show_forum();
void do_forum();
void show_topic();
void do_topic();
void show_error();
void do_error();
void do_pause();

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
	memcpy(currentUrl, "http://cpprograms.net/devnull/nesdev/", FIRST_CUSTOM_URL_CHAR-1);
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

unsigned char wrapped_pad_trigger() {
	lastPadState = _currentPadState;
	_currentPadState = nesnet_pad_poll();
	tempPadState = (_currentPadState ^ lastPadState) & _currentPadState;
	return tempPadState;
}

int wrapped_http_get(char* url, char* buffer, int length) {
	http_get(url, buffer, length);
	while (!http_request_complete()) {
		nesnet_do_cycle();
		ppu_wait_frame();
	}
	return http_response_code();
}

void clear_screen() {
	vram_adr(0x2060);
	vram_fill(0, 0x03a0);
}

// Dumb helper method to show the URL on-screen to help debug
// NOTE: Screen must be off.
void draw_debug_info() {
	// put_str(NTADR_A(1, 27), currentUrl);
}

void show_connection_failure() {
	ppu_off();
	put_str(NTADR_A(2,18), "NESNet device not detected.");
	put_str(NTADR_A(2,20), "Connect to the 2nd");
	put_str(NTADR_A(2,21), "controller port.");
	put_str(NTADR_A(2,24), "Press A to retry.");
	ppu_on_all();
}

// Quick-n-dirty convert integer to string, to show an error code on our error screen.
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
	put_str(NTADR_A(2,9),"Press a to browse the");
	put_str(NTADR_A(2,10),"most recent forum posts.");
	put_str(NTADR_A(2,12), "Start to toggle music");
	put_str(NTADR_A(2,13), "After the Rain by Shiru");
	put_str(NTADR_A(2,18), "Waiting for NESNet...");


	ppu_on_all();//enable rendering
	callCount = i = 0;
	ppu_wait_nmi();

	music_play(0);
 	music_pause(1);

	while(1) {
		switch (gameState) {
			case GAME_STATE_INIT:
				do_init();
				break;
			case GAME_STATE_HOME:
				do_home();
				break;
			case GAME_STATE_FORUM:
				do_forum();
				break;
			case GAME_STATE_TOPIC:
				do_topic();
				break;
			case GAME_STATE_ERROR:
				do_error();
				break;
		} 
	}
}

void do_pause() {
	if (currentPadState & PAD_START) {
		musicIsPaused = !musicIsPaused;
		music_pause(musicIsPaused);
	}
}

void do_init() {
	ppu_wait_frame();
	currentPadState = wrapped_pad_trigger();
	if (currentPadState & PAD_A) {
		if (nesnet_check_connected()) {
			show_home();
		} else {
			show_connection_failure();
		}
	} else {
		set_vram_update(NULL);
		// Poll for the device until we see it connected.
		if (!nesnetConnected && nesnetConnectionAttempts < 5) {
			nesnetConnected = nesnet_check_connected();
			++nesnetConnectionAttempts;
			if (nesnetConnected) {
				// If the device is connected, remove connecting message.
				ppu_off();
				put_str(NTADR_A(2, 18), "                     ");
				ppu_on_all();
			}
		} else if (nesnetConnectionAttempts == 5) {
			show_connection_failure();
			// Increment one more time so we only see this message show up once.
			++nesnetConnectionAttempts;
		}
	}

	do_pause();

}

void hide_pointer() {
	oam_spr(0, 0xff, 0, 0, 0);
}

void show_home() {
	hide_pointer();
	ppu_off();
	clear_screen();
	ppu_on_all();

	write_screen_buffer(2, 12, "Loading...");

	ppu_wait_frame(); // Flush output to make sure we see this on screen first.
	set_vram_update(NULL);

	resCode = wrapped_http_get("http://cpprograms.net/devnull/nesdev/", theMessage, 500);

	if (resCode != 200) {
		show_error();
		return;
	}

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
				forumIds[(currentForumId<<1) + 1] = '\0';
			} else {
				forumIds[(currentForumId<<1) + 1] = *currentChar;
				++currentChar; // Force-skip the colon after this char.

			}
			forumIds[currentForumId<<1] = forumIdA;
			++totalForumCount;
			++currentForumId;
		} else {
			vram_put(*currentChar-0x20);
		}
		++currentChar;
		if (*currentChar == '|') {
			offset += 0x20;
			vram_adr(offset);
			++currentChar;
			forumIdA = forumIdB = 0;
		}
	}

	draw_debug_info();

	ppu_on_all();
	gameState = GAME_STATE_HOME;
}

void do_home() {
	ppu_wait_nmi();
	currentPadState = wrapped_pad_trigger();
	// Undocumented feature - use select to reload.
	if (currentPadState & PAD_SELECT) {
		show_home();
		return;
	} else {
		set_vram_update(NULL);
	}

	if (currentPadState & PAD_UP && currentForumPosition > 0) {
		--currentForumPosition;
	} else if (currentPadState & PAD_DOWN && currentForumPosition < totalForumCount-1) {
		++currentForumPosition;
	}

	if (currentPadState & PAD_A) {
		show_forum();
		return;
	}

	do_pause();
	oam_spr(0, 32 + (currentForumPosition<<3), ARROW_CHR_ID, 0, 0);
}

void show_forum() {
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
	currentUrl[FIRST_CUSTOM_URL_CHAR+4] = forumIds[(currentForumPosition<<1) + 1];
	currentUrl[FIRST_CUSTOM_URL_CHAR+5] = '\0';

	resCode = wrapped_http_get(currentUrl, theMessage, 500);

	if (resCode != 200) {
		show_error();
		return;
	}

	ppu_off();
	clear_screen();
	offset = 0x2081;
	vram_adr(offset);
	currentChar = &theMessage[0];
	forumIdA = 0;
	forumIdB = 0;
	currentForumId = 0;
	j = 0;
	jj = 0;
	ii = 0;
	hasHitColon = FALSE;

	while (*currentChar != '\0') {
		// For now, just skip the id. We'll get there.
		if (!hasHitColon && *currentChar == ':') {
			topicIds[j] = 0;
			for (i = 0; jj + i < ii; ++i) {
				if (theMessage[jj+i] >= '0' && theMessage[jj+i] <= '9') {
					topicIds[j] = (topicIds[j]*10) + (theMessage[jj+i] - '0');
				}
			}
			hasHitColon = TRUE;
		} else if (hasHitColon) {
			vram_put(*currentChar-0x20);
		}
		currentChar++;
		if (*currentChar == '|') {
			hasHitColon = FALSE;
			jj = ii;
			j++;
			offset += 0x20;
			vram_adr(offset);
			currentChar++;
			++ii;
			totalTopicCount++;
		}

		++ii;
	}
	draw_debug_info();

	ppu_on_all();
	gameState = GAME_STATE_FORUM;
}


void do_forum() {
	ppu_wait_frame();
	currentPadState = wrapped_pad_trigger();
	// Undocumented feature - use select to restart.
	if (currentPadState & PAD_SELECT) {
		show_forum();
	} else {
		set_vram_update(NULL);
	}

	if (currentPadState & PAD_UP && currentTopicPosition > 0) {
		currentTopicPosition--;
	} else if (currentPadState & PAD_DOWN && currentTopicPosition < totalTopicCount-1) {
		++currentTopicPosition;
	}

	if (currentPadState & PAD_A) {
		show_topic();
		return;
	}

	// B to go back.
	if (currentPadState & PAD_B) {
		show_home();
		return;
	}

	do_pause();
	oam_spr(0, 32 + (currentTopicPosition<<3), ARROW_CHR_ID, 0, 0);
}

void show_topic() {
	// Little hack to not change the post number if we're staying on the same topic.
	if (gameState != GAME_STATE_TOPIC && gameState != GAME_STATE_ERROR) {
		currentTopicPost = 0;
	}
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
	currentUrl[FIRST_CUSTOM_URL_CHAR+1] = 'p';
	currentUrl[FIRST_CUSTOM_URL_CHAR+2] = '=';
	currentUrl[FIRST_CUSTOM_URL_CHAR+3] = '0' + currentTopicPost / 10;
	currentUrl[FIRST_CUSTOM_URL_CHAR+4] = '0' + currentTopicPost % 10;
	currentUrl[FIRST_CUSTOM_URL_CHAR+5] = '&';
	currentUrl[FIRST_CUSTOM_URL_CHAR+6] = 't';
	currentUrl[FIRST_CUSTOM_URL_CHAR+7] = '=';
	itoa(topicIds[currentTopicPosition], &currentUrl[FIRST_CUSTOM_URL_CHAR+8]);

	resCode = wrapped_http_get(currentUrl, theMessage, 500);

	if (resCode != 200) {
		show_error();
		return;
	}

	ppu_off();
	clear_screen();
	offset = 0x20e0;
	vram_adr(offset);
	currentChar = &theMessage[0];
	j = 0;
	ii = 0;
	forumIdA = forumIdB = 0;
	hasGrabbedCount = hasShownAuthor = hasShownDate = 0;
	vram_adr(0x2081);

	numberOfPosts = 10;
	while (*currentChar != '\0') {
		if (!hasGrabbedCount) {
			if (*currentChar == '|') {
				if (forumIdB == 0) {
					numberOfPosts = forumIdA - '0';
				}
				hasGrabbedCount = 1;
			} else if (forumIdA == 0) {
				forumIdA = *currentChar;
			} else {
				forumIdB = *currentChar;
				numberOfPosts = ((forumIdA - '0') * 10) + (*currentChar - '0');
			}
			++currentChar;
			continue;
		} else if (!hasShownAuthor && *currentChar == '|') {
			hasShownAuthor = 1;
			vram_adr(0x20a1);
			++currentChar;
			continue; // Skip this char but keep going.
		} else if (!hasShownDate && *currentChar == '|') {
			hasShownDate = 1;
			vram_adr(offset);
			++currentChar;
			continue; // Skip this char too. Onto the message.
		} else {
			++ii;
			if (ii % 20 == 0) {
				offset += 0x20;
				if (offset >= 0x2380) 
					break;
			}

			if (*currentChar == '\n') {
				vram_adr(offset);
				currentChar++;
				continue;
			}
		}

		vram_put(*currentChar-0x20);
		++currentChar;
	}
	draw_debug_info();

	screenBuffer[0] = '0' + ((currentTopicPost+1) / 10);
	screenBuffer[1] = '0' + (currentTopicPost+1) % 10;
	screenBuffer[2] = '/';
	screenBuffer[3] = '0' + (numberOfPosts / 10);
	screenBuffer[4] = '0' + (numberOfPosts % 10);
	screenBuffer[5] = '\0';
	put_str(NTADR_A(2, 28), screenBuffer);

	ppu_on_all();
	gameState = GAME_STATE_TOPIC;
}

void do_topic() {
	ppu_wait_frame();
	currentPadState = wrapped_pad_trigger();

	if (currentPadState & PAD_B) {
		show_forum();
	}

	if (currentPadState & PAD_SELECT) {
		show_topic();
	}

	if (currentPadState & PAD_UP && currentTopicPost > 0) {
		currentTopicPost--;
		show_topic();
	} else if (currentPadState & PAD_DOWN && currentTopicPost < numberOfPosts-1) {
		currentTopicPost++;
		show_topic();
	}
	do_pause();
}

void show_error() {
	ppu_off();
	clear_screen();
	hide_pointer();

	put_str(NTADR_A(2,3), "ERROR");
	put_str(NTADR_A(2,5), "Failed to load content.");
	put_str(NTADR_A(2,6), "Press A to retry.");
	screenBuffer[0] = 'C';
	screenBuffer[1] = 'O';
	screenBuffer[2] = 'D';
	screenBuffer[3] = 'E';
	screenBuffer[4] = ':';
	screenBuffer[5] = ' ';
	itoa(resCode, &screenBuffer[6]);
	// All codes are 3 long, unless something goes really, really wrong.. so, just shove the \0 where it makes sense.
	screenBuffer[9] = '\0';
	put_str(NTADR_A(2,7), screenBuffer);

	put_str(NTADR_A(2,9), "Content:");
	put_str(NTADR_A(2,10), theMessage);
	ppu_on_all();
	// Deal with a second error coming from our original error state. :(
	if (gameState != GAME_STATE_ERROR) {
		lastGameState = gameState;
		gameState = GAME_STATE_ERROR;
	}
}

void do_error() {
	ppu_wait_frame();
	currentPadState = wrapped_pad_trigger();

	if (currentPadState & PAD_A) {
		switch (lastGameState) {
			case GAME_STATE_INIT:
				show_home(); // No actual solution here, so let's have a decent fallback. This should never happen.
				break;
			case GAME_STATE_HOME:
				show_home();
				break;
			case GAME_STATE_FORUM:
				show_forum();
				break;
			case GAME_STATE_TOPIC:
				show_topic();
				break;
			case GAME_STATE_ERROR:
				show_error();
				break;
		}
	} else {
		do_pause();
	}
}