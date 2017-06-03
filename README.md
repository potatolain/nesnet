# NESNET [![CircleCI](https://circleci.com/gh/cppchriscpp/nesnet.svg?style=svg)](https://circleci.com/gh/cppchriscpp/nesnet)

Ever want to make your NES go online? Now you can! All it takes is a little C knowledge and some time.

**Important**: This project is very much a work in progress. While this should work for basic cases, it's not what
I'd call production-ready code. Requests will sometimes fail, data may be wrong, etc. This is a hobbyist's toy, 
so please treat it that way. (Note: Don't let this dissuade you from filing bugs, though! I just may not get to them
very quickly.)

### Latest CI Artifacts (master branch)
-  [Photon Firmware](https://circleci-tkn.rhcloud.com/api/v1/project/cppchriscpp/nesnet/tree/master/latest/artifacts/photon_firmware.bin)
-  [Basic Demo Rom](https://circleci-tkn.rhcloud.com/api/v1/project/cppchriscpp/nesnet/tree/master/latest/artifacts/hello.nes)
-  [Forum Browser Rom](https://circleci-tkn.rhcloud.com/api/v1/project/cppchriscpp/nesnet/tree/master/latest/artifacts/browser.nes)

## What is this?

A C library for http requests for the NES. 

## Why is this?

Because I can, darnit! Why not?

In seriousness, I saw the 
[Twitter client](http://nobadmemories.com/connectednes/) that 
[Rachel Weil](http://www.twitter.com/partytimehxlnt)
made, and decided I wanted to take that a step further/crazier.

## What features of http does it support?

Right now the feature list is pretty short. I intend to keep building this up over time, and PRs are welcome!

#### Current Features: 
- http get requests
- http post requests
- http put requests
- http delete requests
- Asynchronous requests alongside game logic. No need to stop the action to send a request!


#### Desired/Upcoming Features: 
- https
- Better stability
- More efficient use of rom space, zeropage and system memory.
- Better demos

## What does using the library look like?

Here's a quick, simple code example: 

```C
int resCode;
char[16] theMessage;
// This triggers the get request, which will run in the background
http_get("http://yoursite.com/time.php", theMessage, 16);
// Now, we have to wait a while. You could do all of this in your game's logic loop instead.
while (!http_request_complete()) {
  // This method should be run once per frame. It does all of the web request stuff in the background.
  nesnet_do_cycle();
  ppu_wait_nmi();
}
if (http_response_code() == 200)
  // theMessage will now contain the first 16 chars of output from time.php on yoursite.com.
```

For more thorough examples, check out the demos/ folder.

# How can I get started?

## What hardware do I need?

First off, you need an original NES console. I can't verify that this will work with anything aside from original hardware.
This library is extremely time sensitive in nature, so anything else might risk changing timings and breaking things.
Don't worry, you won't be making any changes to the NES hardware! 

Next, you need some way to run custom roms on the NES hardware. Something such as the 
[PowerPak](http://www.retrousb.com/product_info.php?cPath=24&products_id=34) 
will work. You can also use supplies from a site like [InfiniteNesLives](http://infiniteneslives.com).

Lastly, you will need a particle photon with a way to connect it to the NES controller port. A broken NES controller or a NES 
extension cord purchased online will do fine. You should be able to pick up both the of these things on Amazon:

[NES Controller Extension Cord](https://www.amazon.com/Retro-Bit-NES-Cable-Extension-6FT/dp/B005IL1E0G)

[Particle Photon](https://www.amazon.com/Particle-PHOTON-Comprehensive-Development-Access/dp/B016YNU1A0)

You basically just need to build the 
[hardware for ConnectedNES](http://nobadmemories.com/connectednes/).

The pictures on that site should give a good indication of how you put this together. If you're using an original
NES controller, connect the NES wires to the photon as follows:

- D1: Red (Clock)
- D2: Orange (Latch)
- D3: Yellow (Data)
- GND: Brown
- VIN: White

## How do I use the library from my C project?

Using the library is pretty simple. If you're doing it from a C project, you'll need to make 3 main changes to set
it up.

1. In your bootstrap (crt0.asm) file, include src/nesnet_zp.asm in the zeropage section.
2. In the same file, include src/nesnet.asm. This is the logic behind the header file below uses.
3. Include nesnet.h in the .c file you want to make web requests in. 

You will also need to set up the particle photon, and flash it with the custom nesnet firmware. Find it on the
releases page. You can flash using `particle-cli`. Use the following steps:
```sh
# Only have to do this the first time you set up. You can run the last command again to update to a newer 
# downloaded firmware. (Also, you can just run `make flash`)
npm install -g particle-cli 
particle login
particle flash your_device_name_here ./photon_firmware.bin
```

After that, use it as you'd use any other library. 

### Available Methods

#### unsigned char nesnet_check_connected()
- **Inputs**: 
  - *N/A*
- **Returns**: 1 if nesnet device is connected and working, 0 otherwise.
- **Description**: Tests the connection to the nesnet device. You should generally run this continuously after
               console starts up, or after some input, and wait until you get a good response. It should be
               run once per frame, like nesnet_do_cycle. You do not need to run nesnet_do_cycle while running this.
               This avoids issues with the photon startup causing bogus results.

#### void http_get(unsigned char* url, unsigned char* buffer, int maxLength)
- **Inputs**: 
  - url: The URL to fetch a request from. Must be http (https/other protocols not supported)
  - buffer: Pointer to a char array in which to store data from the request.
  - maxLength: Maximum number of chars to return. Generally this is the length of your char array.
- **Description**: Triggers an http get request, that will put the response into a character array.

#### void http_delete(unsigned char* url, unsigned char* buffer, int maxLength)
- **Inputs**:
  - url: The URL to send the delete request to. Must be http (https/other protocols not supported)
  - buffer: Pointer to a char array in which to store data from the request.
  - maxLength: Maximum number of chars to return. Generally this is the length of your char array.
- **Description**: Triggers an http delete request, that will put the response into a character array.

#### void http_post(unsigned char *url, unsigned char *data, int data_length, unsigned char *buffer, int max_length)
- **Inputs**:
  - url: The URL to send the post request to. Must be http (https/other protocols not supported)
  - data: Data to include with the post request.
  - data_length: How many bytes to copy from `data`. Should include the null terminator for strings.
  - buffer: Pointer to a char array in which to store data from the request.
  - maxLength: Maximum number of chars to return. Generally this is the length of your char array.
- **Description**: Triggers an http post request, that will put the response into a character array.

#### void http_put(unsigned char *url, unsigned char *data, int data_length, unsigned char *buffer, int max_length)
- **Inputs**:
  - url: The URL to send the put request to. Must be http (https/other protocols not supported)
  - data: Data to include with the post request.
  - data_length: How many bytes to copy from `data`. Should include the null terminator for strings.
  - buffer: Pointer to a char array in which to store data from the request.
  - maxLength: Maximum number of chars to return. Generally this is the length of your char array.
- **Description**: Triggers an http put request, that will put the response into a character array.

#### unsigned char http_request_complete() 
- **Returns**: 1 If the last request has finished (and thus your buffer is safe to use, and http_response_code will
               give the correct results.) 0 otherwise.
- **Description**: Tests to see if the last request made to neslib is complete. Will return 1 if a request is not
                   in progress. Value is 1 before any requests are made.

#### int http_response_code()
- **Returns**: The response code from the last http request made.
- **Description**: Gets the response code from the last http request that nesnet made.

#### unsigned char nesnet_pad_poll()
- **Returns**: Buttons currently pressed by the user.
- **Description**: This method *must be used instead of polling the pad directly or using neslib*! This method is
                   aware of nesnet and avoids corrupting messages to/from the NES.

#### void nesnet_do_cycle()
- **Description**: Must be run once per nmi to do http requests in the background. Also seeds input for
                   nesnet_poad_poll and the like.


## How can I build the photon firmware?

Note that you don't have to do this. You can download the latest artifact from CI (link at top of readme) and flash
it using the steps in the "How do I Use This Library in my C Project" section. That said, it is just about as easy
to do, especially if you are working with it directly.

First, make sure particle-cli is installed globally, and you have logged in. Next, go to the `photon-firmware` 
directory and run `make`. It should be that simple. If you'd like to flash your firmware, you can run `make flash`.
(Note: This assumes your photon is named "hamster_nes" - change the name in photon-firmware/makefile.


## How do I set up a development environment to build one of the demos?

### Operating System Specifics

#### Windows
- Install [Gow](https://github.com/bmatzelle/gow/wiki) (GNU On Windows) or Cygwin to get certain unix commands.
- Download the [older Windows version of cc65](ftp://ftp.musoftware.de/pub/uz/cc65/cc65-win32-2.13.3-1.zip) and extract
  it to the tools/cc65 folder in the demo. tools/cc65/bin/cc65 should be an executable

#### Linux (Tested on Ubuntu)
- Download the [older version of the cc65 source code](ftp://ftp.musoftware.de/pub/uz/cc65/cc65-sources-2.13.3.tar.bz2)
  and extract it to the tools/cc65 folder.
- Build cc65 for your system. Run `make --file="make/gcc.mak"`
- Create the tools/cc65/bin folder manually. You can use this: 
  ```sh
  mkdir -p tools/cc65/bin
  find tools/cc65 -type f -executable -exec cp {} tools/cc65/bin/ \;
  ```

### All Operating Systems

At this point, you should be able to run `make` in the the base folder for the demo and build it. There are other tools
and commands built into the makefile. See the various readme files in the tools folder for more details. A rom file will
be built in the base folder.


# What known caveats/issues does the system have?

- While a request is in progress, your program *must never read input directly*. The library provides a nesnet_pad_poll method
  that caches/times pad inputs without corrupting the request/response.
- Responses will occasionally be off by a few bytes. This generally impacts the status code (200 OK, 404, etc) before anything
  else, so is easy to detect. 
- If a response is too long, the NES library will cut it off, and if another request is made before the photon firmware finishes
  sending the message, that will corrupt the next request.
- Request URLs get corrupted on occasion. This happens more often when music/sound effects are playing.
- The NES Powerpak seems to stop accepting controller input after 5-10 seconds if the photon is connected with the nesnet 
  firmware on it. Either start your game fast, or plug in the photon after choosing a game.
- Flashing the Photon firmware causes the NES to reset, or sometimes end up in an undefined state.
- If the request from the library to the photon takes too long, the library will respond with a 599 error. (Unique to NESNet)

# How does it work? 

There are two major pieces involved in this. One is the assembly library, which is called by your C code and runs on the NES.
The other pieces is the C++ firmware on the particle photon. 

The assembly library takes a URL, and turns it into data the NES can consume. There are a number of steps that make 
this possible.
1. First, the assembly library sends a handshake to the photon, telling it there is a url (and possible playload) coming. 
2. Next, the assembly library sends the message to the nes controller port, one bit at a time using the latch signal.
3. The C++ firmware on the photon reads these messages, and merges each bit back into a bigger byte. It assembles this into
   a url and payload. 
4. The C++ firmware makes a web request to the given URL, and waits for the response. 
5. The C++ firmware encodes this response into a set number of bytes, and truncates as necessary. 
6. The C++ firmware begins sending the message back through the controller one byte at a time.
7. The assembly library reads controller presses into the output buffer as characters. These are your results.

The data coming from the photon firmware into the NES is pretty straightforward, as simply simulating its regular behavior
normally sends a byte of data with each press. We can poll the controller very rapidly, so we can get the response in a
timely manner.

However, sending the URLs through the NES controller is more involved. We can send exactly one signal to the NES controller -
the latch signal, which tells it to start sending keypresses. This is a quick pulse on and off. This can trigger an event, 
but on its own does not seem useful. However, if we consider time as another dimension, we have more options. When the assembly
library needs to send a bit to the C++ firmware, it will send either one latch signal, or two in quick succession, then stop
for a known amount of time. The C++ firmware starts a timer when it gets a latch signal, and sets the current bit to 0. If it
gets another bit within a short timeframe, it changes the current bit to one. After the set time is elapsed, the bit is written
into the current byte, and the process repeats.

This sounds like it would be very slow, and to a degree it is. However, this delay is measured in microseconds for each bit, so
the delay is bearable. (Especially on an older console like the NES. We expect it to be slow!) 

# How is this different from ConnectedNES?

Well, for one it's a lot less pretty. Seriously, I have no balloons or anything in my demos!

Also, ConnectedNES is very purpose-built. The photon software is built to receive events from a node client running on
a server - specifically tailored to your NES. The NES actually receives push events through the controller port whenever
the server sees a new tweet. Configuration (what to search for) is also done entirely on the server side.

This project has no special server software required. It can do regular http requests to any server it can see on the 
internet. The photon software has an http client, and the NES controller port is used to interface with that. It is 
also a library; meant to enable other internet-enabled projects. 

You could use NESNet to build a Twitter client with some effort. You could potentially even send tweets from it!

# How can I contribute?

Just send a PR! There aren't any hard-and-fast rules right now. I'm sure we can work out any changes you might suggest.

# Credits

[ConnectedNES](http://nobadmemories.com/connectednes/) by [Rachel Simone Weil](http://nobadmemories.com) for inspiration and hardware. 

[NESLib](https://shiru.untergrund.net/software.shtml) by [Shiru](http://shiru.untergrund.net/) for basically everything that makes the demo work.

[HttpClient](https://github.com/nmattisson/HttpClient) by [nmattisson](http://nmattisson.com/) for http requests from the photon.

[After The Rain](http://shiru.untergrund.net/music.shtml) by [Shiru](http://shiru.untergrund.net/) for browser music.

If I'm missing anyone/anything, let me know! It's not intentional, I promise.
