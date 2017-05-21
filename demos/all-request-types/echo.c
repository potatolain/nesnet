// Test program for demonstrating NESNet. 
#include "lib/neslib.h"
#include "echo.h"
#include "../../src/nesnet.h"

#define REQUEST_TYPE_NONE 0
#define REQUEST_TYPE_GET 1
#define REQUEST_TYPE_POST 2
#define REQUEST_TYPE_PUT 3
#define REQUEST_TYPE_DELETE 4
#define ECHO_SERVER_URL "http://cpprograms.net/devnull/echo.php"
#define POST_DATA "The quick brown fox jumped over the lasy post request"
#define PUT_DATA "The quick brown fox jumped over the lazy put request"

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
	while (!nesnetConnected) {
		// Automatically try to connect up to 5 times before showing an error.
		if (nesnetConnectionAttempts < 5) {
			nesnetConnected = nesnet_check_connected();
			++nesnetConnectionAttempts;
		} else if (nesnetConnectionAttempts == 5) {
			show_connection_failure();
			++nesnetConnectionAttempts; // Only show the connection failure message once, then bump this up again to avoid repeating.
		}

		// Test for A button - if we failed to connect, this allows us to retry.
		if (pad_trigger(0) & PAD_A) {
			nesnetConnected = nesnet_check_connected();

			// Complain if the connection fails.
			if (!nesnetConnected)
				show_connection_failure();

		}
		ppu_wait_nmi();
	}

	// Clean up everything on the screen before main loop.
	ppu_off();
	show_boilerplate();
	ppu_on_all();

	// Now we wait for input from the user, and do internet-y things!
	while(1) {
		currentPadState = nesnet_pad_poll();
		if (http_request_complete()) {
			if (currentPadState & PAD_UP) {
				requestType = REQUEST_TYPE_GET;
				http_get(ECHO_SERVER_URL"?data=data%20in%20url", theMessage, 64);
			} else if (currentPadState & PAD_DOWN) {
				requestType = REQUEST_TYPE_DELETE;
				http_delete(ECHO_SERVER_URL"?data=data%20in%20url", theMessage, 64);
			} else if (currentPadState & PAD_LEFT) {
				requestType = REQUEST_TYPE_POST;
				http_post(ECHO_SERVER_URL, POST_DATA, sizeof(POST_DATA), theMessage, 64);
			} else if (currentPadState & PAD_RIGHT) {
				requestType = REQUEST_TYPE_PUT;
				http_put(ECHO_SERVER_URL, PUT_DATA, sizeof(PUT_DATA), theMessage, 64);
			} else if (requestType != REQUEST_TYPE_NONE) {
				resCode = http_response_code();
				if (resCode == 200) {
					show_the_message("Success: ");
				} else {
					show_the_message("It went wrong: ");
				}
				// Set request type to none so we can trigger a new request.
				requestType = REQUEST_TYPE_NONE;
			}
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
	put_str(NTADR_A(2,9),"Press a direction to");
	put_str(NTADR_A(2,10),"make a request.");
	put_str(NTADR_A(2,12), "UP:   Get   DOWN:  Delete");
	put_str(NTADR_A(2,13), "LEFT: Post  RIGHT: Put");
}

// If we have a problem connecting to the nesnet device, let the user know.
void show_connection_failure() {
	ppu_off();
	put_str(NTADR_A(2,18), "NESNet device not detected.");
	put_str(NTADR_A(2,20), "Connect it to the 2nd");
	put_str(NTADR_A(2,21), "controller port, then reset");
	put_str(NTADR_A(2,22), "the console.");
	put_str(NTADR_A(2,24), "Press A to try again.");
	ppu_on_all();
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