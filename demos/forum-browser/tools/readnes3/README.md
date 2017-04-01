# ReadNES3

ReadNES3 is a tool that prepares roms for writing to actual carts. It splits chr and prg (irrelevant to us) and also strips headers. 
If installed, running `make prepare_cart` will generate bin files within the bin folder, suitable for writing with 
INL retro-prog or similar programs. 

http://www.racketboy.com/forum/viewtopic.php?f=25&t=28549
https://github.com/AaronBottegal/ReadNES3