# Character Momvement Network Test

This is a more advanced nesnet app. It connects to a server that serves the position of two sprites, 
allowing control of one from the NES, and another from a web browser.

The server source is in the server folder. I may host this some day, but for now the URL is in `world.c`, and 
has to be manually changed. (Look for a define at the top of the file.) Note that you can also connect to it
with a web browser and move the server sprite yourself!

You will need to change the URL in `world.c` to contain the IP of your computer. On startup, the server will 
list all addresses it knows it is listening on. One of these should work - it will often be the one starting 
with 192.168.1. You will have to be connected to the same router as your NES, unless you host this on the
internet.

You can move your sprite by using the control pad. You can also connect to the http address with your web
browser and move a second character there. There is no collision logic. Enjoy.

Sprites stolen from [Missing Lands](http://cpprograms.net/classic-gaming/missing-lands/), 
my Ludum Dare 38 entry.