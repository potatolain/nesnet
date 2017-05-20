# CC65

cc65 is the C compiler for this game. You need it. 

Get it here: [ftp://ftp.musoftware.de/pub/uz/cc65/](ftp://ftp.musoftware.de/pub/uz/cc65/)

## Windows setup

For Windows, grab win32 the binary zip from the *old* cc65 (version 2.13.3-1; before it was taken over) and unzip it here. 

You should see a bin/ folder. That's it!

## Linux setup 

Grab the sources zip for the *old* cc65 (version 2.13.3-1; before it was taken over) and extract it here. You should see an 
`include` folder and `make` folder alongside a few more. If you see a cc65-2.13.3 folder, move its contents up.

Next, build the binaries. 
```sh
make --file="make/gcc.mak"
```

Finally, we need to collect the binaries ourselves. Do that like this:

```sh
mkdir -p tools/cc65/bin
find tools/cc65 -type f -executable -exec cp {} tools/cc65/bin/ \;
```

### Note

My other projects use the latest cc65, so you *can not copy it over from another one.*