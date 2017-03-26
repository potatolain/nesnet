// This is a test program for some NES networking stuff I'm gonna do.
#include "lib/neslib.h"
#include "../src/nesnet.h"

static unsigned char theMessage[16];
static unsigned char screenBuffer[20]; // Same data, but transformed for the screen, and with 3 bytes of prefix for neslib.
static unsigned char currentPadState;
static unsigned char i, callCount;
static unsigned char *ptr;

//put a string into the nametable

void put_str(unsigned int adr,const char *str)
{
	vram_adr(adr);

	while(1)
	{
		if(!*str) break;

		vram_put((*str++)-0x20);//-0x20 because ASCII code 0x20 is placed in tile 0 of the CHR
	}
}


void main(void) {
	//rendering is disabled at the startup, the palette is all black

	pal_col(1,0x30);//set while color


	theMessage[0] = 'P';
	theMessage[1] = 'E';
	theMessage[2] = 'N';
	theMessage[3] = 'G';
	theMessage[4] = 'U';
	theMessage[5] = 'I';
	theMessage[6] = 'N';

	//you can't put data into vram through vram_put while rendering is enabled
	//so you have to disable rendering to put things like text or a level map
	//into the nametable

	//there is a way to update small number of nametable tiles while rendering
	//is enabled, using set_vram_update and an update list

	put_str(NTADR_A(2,2),"HELLO, WORLD!");
	put_str(NTADR_A(2,6),"THIS CODE PRINTS SOME TEXT");
	put_str(NTADR_A(2,7),"USING ASCII-ENCODED CHARSET");
	put_str(NTADR_A(2,8),"(WITH CAPITAL LETTERS ONLY)");
	put_str(NTADR_A(2,10),"TO USE CHR MORE EFFICIENTLY");
	put_str(NTADR_A(2,11),"YOU'D NEED A CUSTOM ENCODING");
	put_str(NTADR_A(2,12),"AND A CONVERSION TABLE");

	put_str(NTADR_A(2, 14), theMessage);

	put_str(NTADR_A(2,16),"CURRENT VIDEO MODE IS");

	if(ppu_system()) put_str(NTADR_A(24,16),"NTSC"); else put_str(NTADR_A(24,16),"PAL");

	ppu_on_all();//enable rendering
	callCount = i = 0;
	ppu_wait_frame();

	while(1) {
		ppu_wait_frame();
		currentPadState = pad_trigger(0);
		if (currentPadState & PAD_A) {
			callCount++;
			http_get("http://cpprograms.net/devnull/time.php", theMessage, 16);
			
			screenBuffer[0] = MSB(NTADR_A(2, 14)) | NT_UPD_HORZ;
			screenBuffer[1] = LSB(NTADR_A(2, 14));
			screenBuffer[2] = 16u;
			for (i = 0u; i < 16u; i++) 
				screenBuffer[i+3u] = theMessage[i]-0x20;
			screenBuffer[19] = NT_UPD_EOF;

			set_vram_update(screenBuffer);
		} else if (currentPadState & PAD_B) {
			screenBuffer[0] = MSB(NTADR_A(2, 14)) | NT_UPD_HORZ;
			screenBuffer[1] = LSB(NTADR_A(2, 14));
			screenBuffer[2] = 16u;
			for (i = 0u; i < 16u; i++) 
				screenBuffer[i+3u] = ' '-0x20;
			screenBuffer[19] = NT_UPD_EOF;

			set_vram_update(screenBuffer);
		} else if (currentPadState & PAD_SELECT) {
			callCount++;
			http_get("http://cpprograms.net:80/devnull/word.php?1=22", theMessage);
			
			screenBuffer[0] = MSB(NTADR_A(2, 14)) | NT_UPD_HORZ;
			screenBuffer[1] = LSB(NTADR_A(2, 14));
			screenBuffer[2] = 16u;
			for (i = 0u; i < 16u; i++) 
				screenBuffer[i+3u] = theMessage[i]-0x20;
			screenBuffer[19] = NT_UPD_EOF;

			set_vram_update(screenBuffer);

		} else {
			set_vram_update(NULL);
		}
	}
}