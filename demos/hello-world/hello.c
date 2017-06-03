// Test program for demonstrating NESNet. 
#include "lib/neslib.h"
#include "hello.h"
#include "../../src/nesnet.h"

#define REQUEST_TYPE_NONE 0
#define REQUEST_TYPE_WORD 1
#define REQUEST_TYPE_IP 2

static unsigned char currentPadState, nesnetConnected, nesnetConnectionAttempts;
static int resCode;
static unsigned char theMessage[64];
static unsigned char requestType;

// Main entry point for the application.
void main(void) {
	nesnetConnected = 0; 
	nesnetConnectionAttempts = 0;
	requestType = REQUEST_TYPE_NONE;

	show_boilerplate();
	put_str(NTADR_A(2,18), "Waiting for NESNet...");
	ppu_on_all();

	// Wait for the NESNet device to be connected
	while (!nesnet_check_connected()) {
		ppu_wait_nmi();
	}

	// Clean up everything on the screen before main loop.
	ppu_off();
	show_boilerplate();
	ppu_on_all();

	// Now we wait for input from the user, and do internet-y things!
	while(1) {
		currentPadState = nesnet_pad_poll();
		if (http_request_complete() && currentPadState & PAD_A) {
			requestType = REQUEST_TYPE_IP;
			http_get("http://ipinfo.io/ip", theMessage, 64);

		} else if (http_request_complete() && currentPadState & PAD_B) {
			requestType = REQUEST_TYPE_WORD;
			http_get("http://www.setgetgo.com/randomword/get.php", theMessage, 64);
		} else if (http_request_complete() && requestType != REQUEST_TYPE_NONE) {
			resCode = http_response_code();
			if (resCode == 200) {
				if (requestType == REQUEST_TYPE_WORD) {
					show_the_message("Random Word:");
				} else {
					show_the_message("Your IP:");
				}
			} else {
				show_the_message("It went wrong: ");
			}
			// Set request type to none so we can trigger a new request.
			requestType = REQUEST_TYPE_NONE;
		}
		nesnet_do_cycle();
		ppu_wait_nmi();
	}
}


// Put a string on the screen at X/Y coordinates given in adr.
void put_str(unsigned int adr, const char *str) {
	vram_adr(adr);
	while(1) {
		if(!*str) break;
		vram_put((*str++)-0x20);//-0x20 because ASCII code 0x20 is placed in tile 0 of the CHR
	}
}

// Show the basic text we show on every screen.
void show_boilerplate() {
	// Clear the screen to start
	vram_adr(0x2060);
	vram_fill(0, 0x03a0);

	// Set a few palette colors so we can see the text we're writing.
	pal_col(1,0x19);
	pal_col(17, 0x19);

	// Show some text we show pretty much everywhere.
	put_str(NTADR_A(7,2),"NESNet Demo!");
	put_str(NTADR_A(7,4), "By cppchriscpp");
	put_str(NTADR_A(3,27), "Inspired by ConnectedNES");
	put_str(NTADR_A(2,9),"Press A to get your public");
	put_str(NTADR_A(2,10),"IP.");
	put_str(NTADR_A(2,12), "Press B to show a random");
	put_str(NTADR_A(2,13), "word via the setgetgo api");
}

// Show whatever message is in the "theMessage" variable, as well as a string passed in to describe that message.
void show_the_message(char* whatIsThis) {
	ppu_off();
	show_boilerplate();
	if (resCode == 200) {
		put_str(NTADR_A(2, 18), whatIsThis);
	} else {
		put_str(NTADR_A(2, 18), "Encountered error getting response:");
	}
	put_str(NTADR_A(2, 20), theMessage);

	ppu_on_all();
}