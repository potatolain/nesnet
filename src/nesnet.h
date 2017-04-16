#ifndef NESNET_H
#define NESNET_H

// Reserving 20 non-zp bytes for internal use. Don't touch these unless you'd like to be sad.
// TODO: Can we drop this to maybe 10 or 5 bytes? Currently only used for the test method.
unsigned char nesnet_buffer[20];

/**
 * Make a generic HTTP request.
 * url: The url to request.
 * buffer: The character array to populate with data
 * max_length: How many bytes to capture at maximum. This should be equal to the size of your buffer char array.
*/
int __fastcall__ http_get(unsigned char* url, unsigned char *buffer, int max_length);

/**
 * Make an HTTP delete request
 * url: The url to request.
 * buffer: The character array to populate with response data.
 * max_length: How many bytes to capture at maximum. This should be equal to the size of your buffer char array.
*/
int __fastcall__ http_delete(unsigned char* url, unsigned char *buffer, int max_length);

/**
 * Make an HTTP POST request.
 * url: The url for the request.
 * data: The data to post to the url.
 * data_length: How many bytes there are to copy in data. Should include the null terminator for strings.
 * buffer: The character array to populate with data.
 * max_length: How many bytes of the response to capture at maximum. This should be the size of your char array.
*/
int __fastcall__ http_post(unsigned char *url, unsigned char *data, int data_length, unsigned char *buffer, int max_length);

/**
 * Make an HTTP PUT request.
 * url: The url for the request.
 * data: The data to put to the url.
 * data_length: How many bytes there are to copy in data. Should include the null terminator for strings.
 * buffer: The character array to populate with data.
 * max_length: How many bytes of the response to capture at maximum. This should be the size of your char array.
*/
int __fastcall__ http_put(unsigned char *url, unsigned char *data, int data_length, unsigned char *buffer, int max_length);


/**
 * Test to make sure a NESNet adapter is connected and responding correctly.
 * Generally it's a good idea to run this at least once during startup (after some input, like hitting start) to make sure it's there.
 */
unsigned char __fastcall__ nesnet_check_connected(void);
#endif