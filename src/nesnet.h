#ifndef NESNET_H
#define NESNET_H
// Reserving 20 non-zp bytes for internal use. Don't touch these unless you'd like to be sad.
unsigned char nesnet_buffer[20];

/**
 * Make a generic HTTP request.
*/
int __fastcall__ http_get(unsigned char* url, unsigned char *buffer);

/**
 * Test to make sure a NESNet adapter is connected and responding correctly.
 * Generally it's a good idea to run this at least once during startup (after some input, like hitting start) to make sure it's there.
 */
unsigned char __fastcall__ nesnet_check_connected(void);
#endif